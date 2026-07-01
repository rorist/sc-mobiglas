#include "draw_utils.h"

void draw_panel_border(GContext *ctx, GRect bounds, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_rect(ctx, bounds);
}

void draw_corner_accents(GContext *ctx, GRect bounds, GColor color) {
  const int leg = 4;
  int x0 = bounds.origin.x;
  int y0 = bounds.origin.y;
  int x1 = x0 + bounds.size.w - 1;
  int y1 = y0 + bounds.size.h - 1;

  graphics_context_set_stroke_color(ctx, color);

  // Top-left
  graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x0 + leg, y0));
  graphics_draw_line(ctx, GPoint(x0, y0), GPoint(x0, y0 + leg));

  // Top-right
  graphics_draw_line(ctx, GPoint(x1 - leg, y0), GPoint(x1, y0));
  graphics_draw_line(ctx, GPoint(x1, y0), GPoint(x1, y0 + leg));

  // Bottom-left
  graphics_draw_line(ctx, GPoint(x0, y1 - leg), GPoint(x0, y1));
  graphics_draw_line(ctx, GPoint(x0, y1), GPoint(x0 + leg, y1));

  // Bottom-right
  graphics_draw_line(ctx, GPoint(x1, y1 - leg), GPoint(x1, y1));
  graphics_draw_line(ctx, GPoint(x1 - leg, y1), GPoint(x1, y1));
}

void draw_panel_header(GContext *ctx, GRect bounds, const char *title,
                       GFont font, GColor color) {
  // Header text area: inset 4px from panel edges, 16px tall
  GRect header_rect = GRect(bounds.origin.x + 4, bounds.origin.y + 2,
                            bounds.size.w - 8, 16);
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, title, font, header_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Underline below header
  int underline_y = bounds.origin.y + 18;
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_line(ctx,
                     GPoint(bounds.origin.x + 2, underline_y),
                     GPoint(bounds.origin.x + bounds.size.w - 3, underline_y));
}

void draw_battery_bar(GContext *ctx, GRect bounds, int percent, GColor color) {
  const int seg_count = 6;
  const int seg_gap = 2;
  int total_w = bounds.size.w;
  int seg_w = (total_w - (seg_count - 1) * seg_gap) / seg_count;
  int filled = (percent * seg_count + 50) / 100;  // round to nearest segment

  for (int i = 0; i < seg_count; i++) {
    int x = bounds.origin.x + i * (seg_w + seg_gap);
    GRect seg_rect = GRect(x, bounds.origin.y, seg_w, bounds.size.h);

    if (i < filled) {
      graphics_context_set_fill_color(ctx, color);
      graphics_fill_rect(ctx, seg_rect, 0, GCornerNone);
    } else {
      graphics_context_set_stroke_color(ctx, color);
      graphics_draw_rect(ctx, seg_rect);
    }
  }
}
