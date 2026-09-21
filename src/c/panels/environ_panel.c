#include "environ_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_temp_buf[12];     // "-12" or "--"
static char s_cond_buf[12];     // "CLOUDY"
static char s_event_buf[20];    // "Meeting..."
static char s_evtime_buf[8];    // "14:30"

static void prv_draw_pipe(GContext *ctx, int x, int y, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y + 8));
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "ENVIRON", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  int x = content.origin.x;
  int y = content.origin.y;
  int w = content.size.w;

  graphics_context_set_text_color(ctx, COLOR_TEXT);

  // Temp label with pipe marker
  prv_draw_pipe(ctx, x, y + 2, COLOR_SECONDARY);
  GRect temp_lbl = GRect(x + 4, y, w - 4, 14);
  graphics_draw_text(ctx, "TEMP", fonts_get(FONT_SIZE_HEADER), temp_lbl,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  // Temp value
  GRect temp_val = GRect(x + 4, y + 10, w - 4, 18);
  graphics_draw_text(ctx, s_temp_buf, fonts_get(FONT_SIZE_VALUE), temp_val,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Condition (inline, smaller)
  GRect cond_rect = GRect(x + 4, y + 28, w - 4, 14);
  graphics_draw_text(ctx, s_cond_buf, fonts_get(FONT_SIZE_HEADER), cond_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Event with pipe marker
  if (s_event_buf[0]) {
    prv_draw_pipe(ctx, x, y + 40, COLOR_SECONDARY);
    GRect evt_rect = GRect(x + 4, y + 42, w - 4, 14);
    graphics_draw_text(ctx, s_event_buf, fonts_get(FONT_SIZE_HEADER), evt_rect,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }

  if (s_evtime_buf[0]) {
    GRect evtime_rect = GRect(x + 4, y + 56, w - 4, 14);
    graphics_draw_text(ctx, s_evtime_buf, fonts_get(FONT_SIZE_HEADER), evtime_rect,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
  }
}

Layer *environ_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);

  snprintf(s_temp_buf, sizeof(s_temp_buf), "--\u00b0C");
  snprintf(s_cond_buf, sizeof(s_cond_buf), "---");
  snprintf(s_event_buf, sizeof(s_event_buf), "EVT: ---");
  s_evtime_buf[0] = '\0';

  return s_layer;
}

void environ_panel_destroy(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void environ_panel_update_bounds(GRect bounds) {
  if (s_layer) {
    layer_set_frame(s_layer, bounds);
    layer_mark_dirty(s_layer);
  }
}

void environ_panel_set_weather(int8_t temp_c, const char *condition) {
  snprintf(s_temp_buf, sizeof(s_temp_buf), "%d\u00b0C", (int)temp_c);
  if (condition) {
    strncpy(s_cond_buf, condition, sizeof(s_cond_buf) - 1);
    s_cond_buf[sizeof(s_cond_buf) - 1] = '\0';
  }
  if (s_layer) layer_mark_dirty(s_layer);
}

void environ_panel_set_event(const char *title, uint32_t event_time) {
  if (title && title[0]) {
    snprintf(s_event_buf, sizeof(s_event_buf), "%.18s", title);
  } else {
    snprintf(s_event_buf, sizeof(s_event_buf), "EVT: ---");
  }

  if (event_time > 0) {
    time_t t = (time_t)event_time;
    struct tm *tm = localtime(&t);
    strftime(s_evtime_buf, sizeof(s_evtime_buf), "%H:%M", tm);
  } else {
    s_evtime_buf[0] = '\0';
  }

  if (s_layer) layer_mark_dirty(s_layer);
}
