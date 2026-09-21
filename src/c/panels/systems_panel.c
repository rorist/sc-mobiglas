#include "systems_panel.h"
#include "panel.h"
#include "../watchface.h"

static Layer *s_layer;
static char s_bat_buf[12];  // "BAT: 99%"

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Battery state + label (rendered right-aligned inside the header)
  BatteryChargeState bat = battery_state_service_peek();
  snprintf(s_bat_buf, sizeof(s_bat_buf), "BAT: %d%%", bat.charge_percent);

  // Semantic color: charging=green, low(<=20%)=warn, else value color
  GColor bat_col = watchface_get_color_value();
  if (bat.is_charging) {
    bat_col = COLOR_SAFE;
  } else if (bat.charge_percent <= 20) {
    bat_col = watchface_get_color_warn();
  }

  // Chrome + header (label left, battery right)
  panel_draw_header_with_right(ctx, bounds, "SYSTEMS", s_bat_buf,
                               COLOR_PRIMARY, bat_col);

  // Battery bar centered in the content area below the header
  GRect content = panel_content_rect(bounds);
  const int bar_h = 8;
  GRect bar_rect = GRect(content.origin.x,
                         content.origin.y + (content.size.h - bar_h) / 2,
                         content.size.w, bar_h);
  draw_battery_bar(ctx, bar_rect, bat.charge_percent, bat_col);
}

Layer *systems_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);
  layer_mark_dirty(s_layer);
  return s_layer;
}

void systems_panel_destroy(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void systems_panel_update_bounds(GRect bounds) {
  if (s_layer) {
    layer_set_frame(s_layer, bounds);
    layer_mark_dirty(s_layer);
  }
}
