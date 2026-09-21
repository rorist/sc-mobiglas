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

// Extended header: accent bar + title (left) + optional right-aligned label
// + partial underline + arrow glyph. right_label == NULL -> title-only layout.
void draw_panel_header_ex(GContext *ctx, GRect bounds, const char *title,
                          const char *right_label, GFont font, GColor color,
                          GColor right_color) {
  int x = bounds.origin.x;
  int y = bounds.origin.y;
  int w = bounds.size.w;

  graphics_context_set_stroke_color(ctx, color);

  // Vertical accent bar: 2px wide, 8px tall, at (x+4, y+3)
  graphics_draw_line(ctx, GPoint(x + 4, y + 3), GPoint(x + 4, y + 10));
  graphics_draw_line(ctx, GPoint(x + 5, y + 3), GPoint(x + 5, y + 10));

  // Right-side diagonal arrow glyph (up-right): at top-right corner
  int bx = x + w - 4;   // shaft top-right (tip)
  int by = y + 4;
  graphics_draw_line(ctx, GPoint(x + w - 10, y + 10), GPoint(bx, by)); // diagonal shaft
  graphics_draw_line(ctx, GPoint(bx, by), GPoint(bx - 4, by)); // left barb
  graphics_draw_line(ctx, GPoint(bx, by), GPoint(bx, by + 4)); // down barb

  // Header text: title left-aligned, optional right_label right-aligned
  graphics_context_set_text_color(ctx, color);
  int title_w = w - 16;  // default zone: x+8 .. x+w-8
  int label_left = -1;
  if (right_label) {
    GRect measure_box = GRect(x + 8, y + 1, w - 24, 14);
    GSize label_size = graphics_text_layout_get_content_size(
        right_label, font, measure_box,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    int label_w = label_size.w;
    label_left = (x + w - 12) - label_w;  // 2px gap before arrow zone
    GRect label_rect = GRect(label_left, y + 1, label_w, 14);
    graphics_context_set_text_color(ctx, right_color);
    graphics_draw_text(ctx, right_label, font, label_rect,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
    graphics_context_set_text_color(ctx, color);
    title_w = (label_left - 4) - (x + 8);
    if (title_w < 0) {
      title_w = 0;
    }
  }
  GRect header_rect = GRect(x + 8, y + 1, title_w, 14);
  graphics_draw_text(ctx, title, font, header_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Partial underline: from accent bar to ~60% width, clipped before right label
  int underline_y = y + 14;
  int line_end = x + 4 + (w - 8) * 6 / 10;
  if (right_label && label_left >= 0 && line_end > label_left - 4) {
    line_end = label_left - 4;
  }
  graphics_draw_line(ctx, GPoint(x + 4, underline_y), GPoint(line_end, underline_y));
}

void draw_panel_header(GContext *ctx, GRect bounds, const char *title,
                       GFont font, GColor color) {
  draw_panel_header_ex(ctx, bounds, title, NULL, font, color, color);
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

// ---------------------------------------------------------------------------
// Weather icons — 8x8px holo-style, color by severity
// CLEAR/CLOUDY/FOG/UNKNOWN cyan, RAIN/SNOW orange (WARN), STORM red (ALERT)
// ---------------------------------------------------------------------------
void draw_weather_icon(GContext *ctx, GPoint origin, int cond_idx) {
  int x = origin.x;
  int y = origin.y;
  GColor col = COLOR_PRIMARY;
  if (cond_idx == 3 || cond_idx == 4) col = COLOR_WARN;       // RAIN / SNOW
  else if (cond_idx == 5) col = COLOR_ALERT;                  // STORM

  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_fill_color(ctx, col);

  switch (cond_idx) {
    case 0:  // CLEAR — hollow circle
      graphics_draw_circle(ctx, GPoint(x + 4, y + 4), 3);
      break;

    case 1:  // CLOUDY — top dome arc + baseline
      graphics_draw_arc(ctx, GRect(x, y + 1, 8, 8),
                        GOvalScaleModeFitCircle,
                        DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(180));
      graphics_draw_line(ctx, GPoint(x, y + 5), GPoint(x + 8, y + 5));
      break;

    case 2:  // FOG — three staggered horizontal lines
      graphics_draw_line(ctx, GPoint(x, y + 1), GPoint(x + 7, y + 1));
      graphics_draw_line(ctx, GPoint(x, y + 4), GPoint(x + 7, y + 4));
      graphics_draw_line(ctx, GPoint(x + 1, y + 7), GPoint(x + 7, y + 7));
      break;

    case 3:  // RAIN — small cloud + 2 slanted drops
      graphics_draw_arc(ctx, GRect(x + 1, y, 6, 6), GOvalScaleModeFitCircle,
                        DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(180));
      graphics_draw_line(ctx, GPoint(x + 1, y + 3), GPoint(x + 7, y + 3));
      graphics_draw_line(ctx, GPoint(x + 3, y + 5), GPoint(x + 2, y + 7));
      graphics_draw_line(ctx, GPoint(x + 5, y + 5), GPoint(x + 4, y + 7));
      break;

    case 4:  // SNOW — 6-armed star
      graphics_draw_line(ctx, GPoint(x + 4, y), GPoint(x + 4, y + 8));
      graphics_draw_line(ctx, GPoint(x + 1, y + 1), GPoint(x + 7, y + 7));
      graphics_draw_line(ctx, GPoint(x + 7, y + 1), GPoint(x + 1, y + 7));
      break;

    case 5:  // STORM — lightning bolt polyline
      graphics_draw_line(ctx, GPoint(x + 5, y), GPoint(x + 2, y + 4));
      graphics_draw_line(ctx, GPoint(x + 2, y + 4), GPoint(x + 4, y + 4));
      graphics_draw_line(ctx, GPoint(x + 4, y + 4), GPoint(x + 2, y + 8));
      break;

    default:  // UNKNOWN — hollow square
      graphics_draw_rect(ctx, GRect(x + 1, y + 1, 6, 6));
      break;
  }
}

// ---------------------------------------------------------------------------
// Sun icons — 8x8px holo-style for the sunrise/sunset line
// sunset=false: sun (hollow circle + 8 rays), sunset=true: half moon
// ---------------------------------------------------------------------------
void draw_sun_icon(GContext *ctx, GPoint origin, bool sunset) {
  int x = origin.x;
  int y = origin.y;
  graphics_context_set_stroke_color(ctx, COLOR_PRIMARY);

  if (!sunset) {
    // Sun — hollow circle r2 centered (4,4) + 8 rays
    graphics_draw_circle(ctx, GPoint(x + 4, y + 4), 2);
    graphics_draw_line(ctx, GPoint(x + 4, y + 0), GPoint(x + 4, y + 1));
    graphics_draw_line(ctx, GPoint(x + 4, y + 6), GPoint(x + 4, y + 7));
    graphics_draw_line(ctx, GPoint(x + 0, y + 4), GPoint(x + 1, y + 4));
    graphics_draw_line(ctx, GPoint(x + 6, y + 4), GPoint(x + 7, y + 4));
    graphics_draw_line(ctx, GPoint(x + 1, y + 1), GPoint(x + 2, y + 2));
    graphics_draw_line(ctx, GPoint(x + 6, y + 2), GPoint(x + 7, y + 1));
    graphics_draw_line(ctx, GPoint(x + 1, y + 7), GPoint(x + 2, y + 6));
    graphics_draw_line(ctx, GPoint(x + 6, y + 6), GPoint(x + 7, y + 7));
  } else {
    // Moon — right-half arc + vertical chord
    graphics_draw_arc(ctx, GRect(x + 2, y + 2, 4, 4), GOvalScaleModeFitCircle,
                      DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(180));
    graphics_draw_line(ctx, GPoint(x + 4, y + 2), GPoint(x + 4, y + 6));
  }
}

// ---------------------------------------------------------------------------
// Drop icon — 8x8px hollow teardrop, point at top
// ---------------------------------------------------------------------------
void draw_drop_icon(GContext *ctx, GPoint origin) {
  int x = origin.x;
  int y = origin.y;
  graphics_context_set_stroke_color(ctx, COLOR_PRIMARY);

  // Pointed top converging to the bowl
  graphics_draw_line(ctx, GPoint(x + 4, y + 1), GPoint(x + 2, y + 5));
  graphics_draw_line(ctx, GPoint(x + 4, y + 1), GPoint(x + 6, y + 5));
  // Bottom bowl — semicircle right→bottom→left
  graphics_draw_arc(ctx, GRect(x + 2, y + 3, 4, 4), GOvalScaleModeFitCircle,
                    DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));
}

// ---------------------------------------------------------------------------
// Wind icon — 8x8px NE arrow (matches compass dir text)
// ---------------------------------------------------------------------------
void draw_wind_icon(GContext *ctx, GPoint origin) {
  int x = origin.x;
  int y = origin.y;
  graphics_context_set_stroke_color(ctx, COLOR_PRIMARY);

  // Shaft diagonal bottom-left → top-right
  graphics_draw_line(ctx, GPoint(x + 1, y + 6), GPoint(x + 6, y + 1));
  // Arrowhead — horizontal-left and vertical-down from tip
  graphics_draw_line(ctx, GPoint(x + 6, y + 1), GPoint(x + 3, y + 1));
  graphics_draw_line(ctx, GPoint(x + 6, y + 1), GPoint(x + 6, y + 4));
}
