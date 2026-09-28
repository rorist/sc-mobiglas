#include "systems_panel.h"
#include "panel.h"
#include "../watchface.h"
#include "../ui/draw_utils.h"

static Layer *s_layer;
static char s_bat_buf[12];  // "BAT 100%"
static char s_com_buf[8];   // "COM OK" / "COM ERR"

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Chrome + header (title only — metrics live on the content line)
  panel_draw_header_full(ctx, bounds, "SYSTEMS", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  const uint32_t mask = watchface_get_sys_metrics();
  const bool show_bat = (mask & 0x01) != 0;
  const bool show_com = (mask & 0x02) != 0;
  if (!show_bat && !show_com) return;  // header only

  // Battery state
  BatteryChargeState bat = battery_state_service_peek();
  snprintf(s_bat_buf, sizeof(s_bat_buf), "BAT %d%%", bat.charge_percent);
  GColor bat_col = watchface_get_color_value();
  if (bat.is_charging) {
    bat_col = COLOR_SAFE;
  } else if (bat.charge_percent <= 20) {
    bat_col = watchface_get_color_warn();
  }

  // Bluetooth state
  bool bt = bluetooth_connection_service_peek();
  snprintf(s_com_buf, sizeof(s_com_buf), "%s", bt ? "COM OK" : "COM ERR");
  GColor com_col = bt ? watchface_get_color_value()
                      : watchface_get_color_warn();

  // Single line: BAT text | battery bar | COM icon + text
  const int line_h = 14;
  const int l14 = FONT_LEADING_14;
  GFont font = fonts_get(FONT_SIZE_HEADER);
  int y = content.origin.y + (content.size.h - line_h) / 2;
  if (y < content.origin.y) y = content.origin.y;

  GRect measure = GRect(content.origin.x, y - l14, content.size.w, line_h + l14);
  const int icon_gap = 2;
  int com_w = 0;
  if (show_com) {
    GSize com_size = graphics_text_layout_get_content_size(
        s_com_buf, font, measure, GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    com_w = 8 + icon_gap + com_size.w;
  }

  // COM: right-aligned group (icon + text)
  int com_x = content.origin.x + content.size.w - com_w;
  if (show_com) {
    graphics_context_set_stroke_color(ctx, watchface_get_color_label());
    draw_comm_icon(ctx, GPoint(com_x, y + 3));
    graphics_context_set_text_color(ctx, com_col);
    graphics_draw_text(ctx, s_com_buf, font,
                       GRect(com_x + 8 + icon_gap, y - l14,
                             com_w - 8 - icon_gap, line_h + l14),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                       NULL);
  }

  if (show_bat) {
    GSize bat_size = graphics_text_layout_get_content_size(
        s_bat_buf, font, measure, GTextOverflowModeTrailingEllipsis,
        GTextAlignmentLeft);
    // BAT: left-aligned text
    graphics_context_set_text_color(ctx, bat_col);
    graphics_draw_text(ctx, s_bat_buf, font,
                       GRect(content.origin.x, y - l14, bat_size.w,
                             line_h + l14),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                       NULL);

    // Battery bar fills the space between BAT text and COM group
    const int gap = 6;
    int bar_x = content.origin.x + bat_size.w + gap;
    int bar_w = (show_com ? com_x - gap : content.origin.x + content.size.w)
                - bar_x;
    if (bar_w < 20) bar_w = 20;
    const int bar_h = 8;
    GRect bar_rect = GRect(bar_x, y + (line_h - bar_h) / 2, bar_w, bar_h);
    draw_battery_bar(ctx, bar_rect, bat.charge_percent, bat_col);
  }
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

