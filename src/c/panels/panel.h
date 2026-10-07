#pragma once

#include <pebble.h>
#include "../watchface.h"
#include "../ui/draw_utils.h"
#include "../ui/fonts.h"

// ---------------------------------------------------------------------------
// Shared panel drawing helpers
// ---------------------------------------------------------------------------

// Draw the full panel chrome: fill + border on rectangular displays
// (emery/flint). On round (gabbro) the background is drawn once by the root
// layer and separators structure the layout — per-panel chrome is a no-op
// (headers are drawn separately).
#ifdef PBL_ROUND
static inline void panel_draw_chrome(GContext *ctx, GRect bounds, GColor color) {
  (void)ctx;
  (void)bounds;
  (void)color;
}
#else
static inline void panel_draw_chrome(GContext *ctx, GRect bounds, GColor color) {
  draw_panel_fill(ctx, bounds, COLOR_PANEL_BG);
  draw_panel_border(ctx, bounds, color);
}
#endif

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

// Content area: the usable rect below the header (3px under the underline
// on emery/gabbro, 1px on flint which keeps the tight budget)
static inline GRect panel_content_rect(GRect bounds) {
#if PBL_DISPLAY_WIDTH < 200
  // Flint: short panels — no bottom pad, content must fit the tight height
  return GRect(bounds.origin.x + 4, bounds.origin.y + 16,
               bounds.size.w - 8, bounds.size.h - 16);
#else
  // 3px clear below the header underline (y+14): content starts y+18
  return GRect(bounds.origin.x + 4, bounds.origin.y + 18,
               bounds.size.w - 8, bounds.size.h - 22);
#endif
}
