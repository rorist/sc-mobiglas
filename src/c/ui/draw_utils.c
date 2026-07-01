#include "draw_utils.h"

void draw_panel_fill(GContext *ctx, GRect bounds, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void draw_panel_border(GContext *ctx, GRect bounds, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_rect(ctx, bounds);
}

void draw_corner_accents(GContext *ctx, GRect bounds, GColor color) {
  const int leg = 6;
  const int inset = 1;
  int x0 = bounds.origin.x + inset;
  int y0 = bounds.origin.y + inset;
  int x1 = x0 + bounds.size.w - 1 - 2 * inset;
  int y1 = y0 + bounds.size.h - 1 - 2 * inset;

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
  int x = bounds.origin.x;
  int y = bounds.origin.y;
  int w = bounds.size.w;

  graphics_context_set_stroke_color(ctx, color);

  // Vertical accent bar: 2px wide, 10px tall, at (x+4, y+4)
  graphics_draw_line(ctx, GPoint(x + 4, y + 4), GPoint(x + 4, y + 13));
  graphics_draw_line(ctx, GPoint(x + 5, y + 4), GPoint(x + 5, y + 13));

  // Header text after accent bar
  GRect header_rect = GRect(x + 8, y + 2, w - 16, 16);
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, title, font, header_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Partial underline: from accent bar to ~60% width
  int underline_y = y + 18;
  int line_end = x + 4 + (w - 8) * 6 / 10;
  graphics_draw_line(ctx, GPoint(x + 4, underline_y), GPoint(line_end, underline_y));

  // Right-side arrow glyph: small > at (w-8, y+6)
  int ax = x + w - 8;
  int ay = y + 6;
  graphics_draw_line(ctx, GPoint(ax, ay), GPoint(ax + 3, ay + 3));
  graphics_draw_line(ctx, GPoint(ax + 3, ay + 3), GPoint(ax, ay + 6));
}

void draw_battery_bar(GContext *ctx, GRect bounds, int percent, GColor color) {
  const int seg_count = 6;
  const int seg_gap = 2;
  int total_w = bounds.size.w;
  int seg_w = (total_w - (seg_count - 1) * seg_gap) / seg_count;
  int filled = (percent * seg_count + 50) / 100;

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
