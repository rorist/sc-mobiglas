#include "draw_utils.h"

void draw_panel_fill(GContext *ctx, GRect bounds, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, bounds, 3, GCornersAll);
}

void draw_panel_border(GContext *ctx, GRect bounds, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_draw_round_rect(ctx, bounds, 3);
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

  // Vertical accent bar: 2px wide, 8px tall, at (x+4, y+3)
  graphics_draw_line(ctx, GPoint(x + 4, y + 3), GPoint(x + 4, y + 10));
  graphics_draw_line(ctx, GPoint(x + 5, y + 3), GPoint(x + 5, y + 10));

  // Header text after accent bar
  GRect header_rect = GRect(x + 8, y + 1, w - 16, 14);
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, title, font, header_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Partial underline: from accent bar to ~60% width
  int underline_y = y + 14;
  int line_end = x + 4 + (w - 8) * 6 / 10;
  graphics_draw_line(ctx, GPoint(x + 4, underline_y), GPoint(line_end, underline_y));

  // Right-side diagonal arrow glyph (up-right): at top-right corner
  int ax = x + w - 10;  // shaft bottom-left
  int ay = y + 10;
  int bx = x + w - 4;   // shaft top-right (tip)
  int by = y + 4;
  graphics_draw_line(ctx, GPoint(ax, ay), GPoint(bx, by));   // diagonal shaft
  graphics_draw_line(ctx, GPoint(bx, by), GPoint(bx - 4, by)); // left barb
  graphics_draw_line(ctx, GPoint(bx, by), GPoint(bx, by + 4)); // down barb
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

void draw_ring_gauge(GContext *ctx, GRect box, int percent,
                     uint16_t thickness, GColor track, GColor fill) {
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;

  // Track (full ring)
  graphics_context_set_fill_color(ctx, track);
  graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thickness,
                       DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

  // Filled portion
  if (percent > 0) {
    graphics_context_set_fill_color(ctx, fill);
    graphics_fill_radial(ctx, box, GOvalScaleModeFitCircle, thickness,
                         DEG_TO_TRIGANGLE(0),
                         DEG_TO_TRIGANGLE((percent * 360) / 100));
  }
}

void draw_dual_ring(GContext *ctx, GRect box,
                    int outer_pct, GColor outer_col,
                    int inner_pct, GColor inner_col) {
  const uint16_t thickness = 4;
  const int innerpad = thickness + 3;  // gap between rings

  // Outer ring
  draw_ring_gauge(ctx, box, outer_pct, thickness, COLOR_GAUGE_BG, outer_col);

  // Inner ring (reduced box)
  GRect inner = GRect(box.origin.x + innerpad, box.origin.y + innerpad,
                      box.size.w - 2 * innerpad, box.size.h - 2 * innerpad);
  draw_ring_gauge(ctx, inner, inner_pct, thickness, COLOR_GAUGE_BG, inner_col);
}
