#include "watchface.h"
#include "panels/time_panel.h"
#include "panels/medical_panel.h"
#include "panels/environ_panel.h"
#include "panels/systems_panel.h"
#include "ui/fonts.h"

static Layer *s_root_layer;
static Layer *s_panel_layers[PANEL_COUNT];
static uint8_t s_config = CONFIG_DEFAULT;
static GRect s_screen_bounds;

// ---------------------------------------------------------------------------
// Root layer — fills background
// ---------------------------------------------------------------------------
static void prv_root_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Bottom accent line
  graphics_context_set_stroke_color(ctx, COLOR_PRIMARY);
  graphics_draw_line(ctx, GPoint(0, 225), GPoint(SCREEN_W - 1, 225));
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
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void watchface_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  s_screen_bounds = layer_get_bounds(window_layer);

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

  if (s_root_layer) {
    layer_destroy(s_root_layer);
    s_root_layer = NULL;
  }

  fonts_deinit();
}

void watchface_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_panel_update(tick_time);
  // Medical panel refreshes health data on its own schedule
  // Systems panel auto-refreshes battery on dirty
  if (s_panel_layers[PANEL_SYSTEMS]) {
    layer_mark_dirty(s_panel_layers[PANEL_SYSTEMS]);
  }
}

uint8_t watchface_get_config(void) {
  return s_config;
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
