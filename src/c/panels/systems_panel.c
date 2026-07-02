#include "systems_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_bat_buf[12];  // "BAT: 99%"

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Chrome + header
  panel_draw_header_full(ctx, bounds, "SYSTEMS", COLOR_PRIMARY);

  // Content area below header
  GRect content = panel_content_rect(bounds);

  // Battery percentage
  BatteryChargeState bat = battery_state_service_peek();
  snprintf(s_bat_buf, sizeof(s_bat_buf), "BAT: %d%%", bat.charge_percent);

  // Semantic color: charging=green, low(<=20%)=orange, else primary
  GColor bat_col = COLOR_PRIMARY;
  if (bat.is_charging) {
    bat_col = COLOR_SAFE;
  } else if (bat.charge_percent <= 20) {
    bat_col = COLOR_WARN;
  }

  // Battery bar: upper portion of content area
  GRect bar_rect = GRect(content.origin.x, content.origin.y + 2,
                          content.size.w, 8);
  draw_battery_bar(ctx, bar_rect, bat.charge_percent, bat_col);

  // Battery text below bar
  GRect text_rect = GRect(content.origin.x, content.origin.y + 12,
                           content.size.w, 16);
  graphics_context_set_text_color(ctx, COLOR_TEXT);
  graphics_draw_text(ctx, s_bat_buf, fonts_get(FONT_SIZE_HEADER), text_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
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
