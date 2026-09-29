#include "watchface.h"
#include "panels/time_panel.h"
#include "panels/medical_panel.h"
#include "panels/environ_panel.h"
#include "panels/systems_panel.h"
#include "ui/fonts.h"
#include "data/storage.h"
#include "data/appmessage.h"

static Layer *s_root_layer;
static Layer *s_panel_layers[PANEL_COUNT];
static uint8_t s_config = CONFIG_DEFAULT;
static uint8_t s_logo = 0;
static bool s_show_date = true;
static uint32_t s_med_mask = 0x03;   // BPM + STEPS
static uint32_t s_env_mask = 0x3F;   // all 6
static uint32_t s_sys_mask = 0x03;   // BAT + COM
static GRect s_screen_bounds;

// Configurable text colors
static GColor s_color_time = COLOR_TIME_DEFAULT;
static GColor s_color_value = COLOR_VALUE_DEFAULT;
static GColor s_color_label = COLOR_LABEL_DEFAULT;
static GColor s_color_header = COLOR_HEADER_DEFAULT;
static GColor s_color_warn = COLOR_WARN_DEFAULT;

// Round chrome (gabbro): the background is drawn once by the root layer;
// this top layer adds the cyan separators — horizontal lines between rows
// (the bezel clips the ends) and a vertical delimiter between MEDICAL and
// ENVIRON, inset 2px from the horizontal lines (never on the outer edges).
#ifdef PBL_ROUND
static Layer *s_sep_layer;
static LayoutInfo s_layout;
static void prv_sep_update_proc(Layer *layer, GContext *ctx) {
  (void)layer;
  graphics_context_set_stroke_color(ctx, COLOR_PRIMARY);
  const GRect t = s_layout.rects[PANEL_TIME];
  const bool has_med = s_layout.visible[PANEL_MEDICAL];
  const bool has_env = s_layout.visible[PANEL_ENVIRON];
  const bool has_mid = has_med || has_env;
  const bool has_sys = s_layout.visible[PANEL_SYSTEMS];
  const int full_w = PBL_DISPLAY_WIDTH;

  if (has_mid || has_sys) {
    const int y = t.origin.y + t.size.h + PANEL_GAP / 2;
    graphics_draw_line(ctx, GPoint(0, y), GPoint(full_w - 1, y));
  }
  if (has_mid && has_sys) {
    const GRect m = has_med ? s_layout.rects[PANEL_MEDICAL]
                            : s_layout.rects[PANEL_ENVIRON];
    const int y = m.origin.y + m.size.h + PANEL_GAP / 2;
    graphics_draw_line(ctx, GPoint(0, y), GPoint(full_w - 1, y));
  }
  if (has_med && has_env) {
    const GRect m = s_layout.rects[PANEL_MEDICAL];
    const int x = m.origin.x + m.size.w + PANEL_GAP / 2;
    const int y_top = t.origin.y + t.size.h + PANEL_GAP / 2 + 4;
    const int y_bot = has_sys ? m.origin.y + m.size.h + PANEL_GAP / 2 - 4
                              : m.origin.y + m.size.h - 1;
    graphics_draw_line(ctx, GPoint(x, y_top), GPoint(x, y_bot));
  }
}
#endif

// ---------------------------------------------------------------------------
// Root layer — fills background
// ---------------------------------------------------------------------------
static void prv_root_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
#ifdef PBL_ROUND
  // Round (gabbro): full-screen panel-blue background; the separator layer
  // structures the layout with cyan lines (bezel clips the line ends).
  graphics_context_set_fill_color(ctx, COLOR_PANEL_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#else
  // Rect displays (emery/flint): plain black background; per-panel chrome
  // (fill + border) lives in panel.h again. No bottom accent line — the
  // SYSTEMS panel border already marks the bottom edge.
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#endif
}

// ---------------------------------------------------------------------------
// Rebuild all panels from scratch based on current config
// ---------------------------------------------------------------------------
static void prv_rebuild_panels(void) {
  // Destroy existing panels
  for (int i = 0; i < PANEL_COUNT; i++) {
    if (s_panel_layers[i]) {
      layer_remove_from_parent(s_panel_layers[i]);
    }
  }
  time_panel_destroy();
  medical_panel_destroy();
  environ_panel_destroy();
  systems_panel_destroy();
  memset(s_panel_layers, 0, sizeof(s_panel_layers));

  // Compute new layout
  LayoutInfo layout = layout_compute(s_screen_bounds, s_config);

  // Create visible panels
  // TIME — always visible
  s_panel_layers[PANEL_TIME] = time_panel_create(layout.rects[PANEL_TIME]);
  layer_add_child(s_root_layer, s_panel_layers[PANEL_TIME]);

  if (layout.visible[PANEL_MEDICAL]) {
    s_panel_layers[PANEL_MEDICAL] = medical_panel_create(layout.rects[PANEL_MEDICAL]);
    layer_add_child(s_root_layer, s_panel_layers[PANEL_MEDICAL]);
  }

  if (layout.visible[PANEL_ENVIRON]) {
    s_panel_layers[PANEL_ENVIRON] = environ_panel_create(layout.rects[PANEL_ENVIRON]);
    layer_add_child(s_root_layer, s_panel_layers[PANEL_ENVIRON]);
  }

  if (layout.visible[PANEL_SYSTEMS]) {
    s_panel_layers[PANEL_SYSTEMS] = systems_panel_create(layout.rects[PANEL_SYSTEMS]);
    layer_add_child(s_root_layer, s_panel_layers[PANEL_SYSTEMS]);
  }

#ifdef PBL_ROUND
  // Separator layer on top of the background (destroyed/recreated with the
  // panels so it stays above them)
  s_layout = layout;
  if (s_sep_layer) {
    layer_remove_from_parent(s_sep_layer);
    layer_destroy(s_sep_layer);
  }
  s_sep_layer = layer_create(s_screen_bounds);
  layer_set_update_proc(s_sep_layer, prv_sep_update_proc);
  layer_add_child(s_root_layer, s_sep_layer);
#endif
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void watchface_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  s_screen_bounds = layer_get_bounds(window_layer);

  // Restore persisted settings (config bitmask, logo, date, text colors)
  s_config = storage_load_config();
  s_logo = storage_load_logo();
  s_show_date = storage_load_show_date() != 0;
  s_med_mask = storage_load_med_metrics();
  s_env_mask = storage_load_env_metrics();
  s_sys_mask = storage_load_sys_metrics();
  s_color_time = storage_load_color_time();
  s_color_value = storage_load_color_value();
  s_color_label = storage_load_color_label();
  s_color_header = storage_load_color_header();
  s_color_warn = storage_load_color_warn();

  // Load fonts
  fonts_init();

  // Root layer
  s_root_layer = layer_create(s_screen_bounds);
  layer_set_update_proc(s_root_layer, prv_root_update_proc);
  layer_add_child(window_layer, s_root_layer);

  // Build panels based on default config
  prv_rebuild_panels();
}

void watchface_destroy(void) {
  time_panel_destroy();
  medical_panel_destroy();
  environ_panel_destroy();
  systems_panel_destroy();
  memset(s_panel_layers, 0, sizeof(s_panel_layers));

#ifdef PBL_ROUND
  if (s_sep_layer) {
    layer_destroy(s_sep_layer);
    s_sep_layer = NULL;
  }
#endif

  if (s_root_layer) {
    layer_destroy(s_root_layer);
    s_root_layer = NULL;
  }

  fonts_deinit();
}

void watchface_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_panel_update(tick_time);
  // MEDICAL re-reads health in its update proc when marked dirty;
  // SYSTEMS auto-refreshes battery on dirty.
  // (App is already awake for the time update — no extra wakeups.)
  if (s_panel_layers[PANEL_MEDICAL]) {
    layer_mark_dirty(s_panel_layers[PANEL_MEDICAL]);
  }
  if (s_panel_layers[PANEL_SYSTEMS]) {
    layer_mark_dirty(s_panel_layers[PANEL_SYSTEMS]);
  }
  // Watch-driven weather refresh every 30 min (tick is reliable,
  // phone-side setInterval is not — PKJS can be killed by Android)
  if (tick_time->tm_min % 30 == 0) {
    appmessage_request_weather();
  }
}

uint8_t watchface_get_config(void) {
  return s_config;
}

uint8_t watchface_get_logo(void) {
  return s_logo;
}

void watchface_set_logo(uint8_t logo) {
  if (logo == s_logo) return;
  s_logo = logo;
  time_panel_refresh();
}

bool watchface_get_show_date(void) {
  return s_show_date;
}

void watchface_set_show_date(bool show) {
  if (show == s_show_date) return;
  s_show_date = show;
  time_panel_refresh();
}

// ---------------------------------------------------------------------------
// Per-panel metrics masks
// ---------------------------------------------------------------------------

uint32_t watchface_get_med_metrics(void) {
  return s_med_mask;
}

uint32_t watchface_get_env_metrics(void) {
  return s_env_mask;
}

uint32_t watchface_get_sys_metrics(void) {
  return s_sys_mask;
}

static void prv_mark_panel(int id) {
  if (s_panel_layers[id]) layer_mark_dirty(s_panel_layers[id]);
}

void watchface_set_med_metrics(uint32_t mask) {
  if (mask == s_med_mask) return;
  s_med_mask = mask;
  prv_mark_panel(PANEL_MEDICAL);
}

void watchface_set_env_metrics(uint32_t mask) {
  if (mask == s_env_mask) return;
  s_env_mask = mask;
  prv_mark_panel(PANEL_ENVIRON);
}

void watchface_set_sys_metrics(uint32_t mask) {
  if (mask == s_sys_mask) return;
  s_sys_mask = mask;
  prv_mark_panel(PANEL_SYSTEMS);
}

// ---------------------------------------------------------------------------
// Configurable text colors
// ---------------------------------------------------------------------------

static void prv_mark_all_dirty(void) {
  for (int i = 0; i < PANEL_COUNT; i++) {
    if (s_panel_layers[i]) layer_mark_dirty(s_panel_layers[i]);
  }
}

// B&W (flint): clamp user-configurable colors to white — dark shades
// would be invisible on the black background. Single point covering
// storage + AppMessage + Clay paths.
static GColor prv_clamp_bw(GColor c) {
  return PBL_IF_BW_ELSE(GColorWhite, c);
}

GColor watchface_get_color_time(void) { return prv_clamp_bw(s_color_time); }
GColor watchface_get_color_value(void) { return prv_clamp_bw(s_color_value); }
GColor watchface_get_color_label(void) { return prv_clamp_bw(s_color_label); }
GColor watchface_get_color_header(void) { return prv_clamp_bw(s_color_header); }
GColor watchface_get_color_warn(void) { return prv_clamp_bw(s_color_warn); }

void watchface_set_color_time(GColor color) {
  s_color_time = color;
  prv_mark_all_dirty();
}

void watchface_set_color_value(GColor color) {
  s_color_value = color;
  prv_mark_all_dirty();
}

void watchface_set_color_label(GColor color) {
  s_color_label = color;
  prv_mark_all_dirty();
}

void watchface_set_color_header(GColor color) {
  s_color_header = color;
  prv_mark_all_dirty();
}

void watchface_set_color_warn(GColor color) {
  s_color_warn = color;
  prv_mark_all_dirty();
}

void watchface_update_config(uint8_t config) {
  if (config == s_config) return;
  s_config = config;
  prv_rebuild_panels();
  environ_panel_refresh_config();  // re-format temp (°C/°F) from last data

  // Trigger immediate time update
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  time_panel_update(t);
}
