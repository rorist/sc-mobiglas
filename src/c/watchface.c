#include "watchface.h"
#include "panels/time_panel.h"
#include "ui/fonts.h"

static Layer *s_root_layer;
static Layer *s_time_layer;

static void prv_root_update_proc(Layer *layer, GContext *ctx) {
  // Fill entire background with near-black
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, COLOR_BG);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void watchface_create(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Load fonts
  fonts_init();

  // Root layer — fills background
  s_root_layer = layer_create(window_bounds);
  layer_set_update_proc(s_root_layer, prv_root_update_proc);
  layer_add_child(window_layer, s_root_layer);

  // TIME panel
  s_time_layer = time_panel_create(RECT_TIME);
  layer_add_child(s_root_layer, s_time_layer);
}

void watchface_destroy(void) {
  time_panel_destroy();

  if (s_root_layer) {
    layer_destroy(s_root_layer);
    s_root_layer = NULL;
  }

  fonts_deinit();
}

void watchface_tick(struct tm *tick_time, TimeUnits units_changed) {
  time_panel_update(tick_time);
}
