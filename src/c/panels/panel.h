#pragma once

#include <pebble.h>
#include "../watchface.h"
#include "../ui/draw_utils.h"
#include "../ui/fonts.h"

// ---------------------------------------------------------------------------
// Shared panel drawing helpers
// ---------------------------------------------------------------------------

// Draw the full panel chrome: fill + border + corner accents
static inline void panel_draw_chrome(GContext *ctx, GRect bounds, GColor color) {
  draw_panel_fill(ctx, bounds, COLOR_PANEL_BG);
  draw_panel_border(ctx, bounds, color);
}

// Draw chrome + header label (fill, border, accents, accent bar, title)
// color = chrome/border color; header text uses configurable header color
static inline void panel_draw_header_full(GContext *ctx, GRect bounds,
                                          const char *title, GColor color) {
  panel_draw_chrome(ctx, bounds, color);
  draw_panel_header(ctx, bounds, title, fonts_get(FONT_SIZE_HEADER),
                    watchface_get_color_header());
}

// Draw chrome + header with optional right-aligned label
// color = chrome/border color; header text uses configurable header color;
// right_color = color of the right label (e.g. battery value)
static inline void panel_draw_header_with_right(GContext *ctx, GRect bounds,
                                                const char *title,
                                                const char *right_label,
                                                GColor color,
                                                GColor right_color) {
  panel_draw_chrome(ctx, bounds, color);
  draw_panel_header_ex(ctx, bounds, title, right_label,
                       fonts_get(FONT_SIZE_HEADER),
                       watchface_get_color_header(), right_color);
}

// Content area: the usable rect below the header (below underline at y+16)
static inline GRect panel_content_rect(GRect bounds) {
  return GRect(bounds.origin.x + 4, bounds.origin.y + 16,
               bounds.size.w - 8, bounds.size.h - 20);
}
