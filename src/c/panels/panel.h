#pragma once

#include <pebble.h>
#include "../watchface.h"
#include "../ui/draw_utils.h"
#include "../ui/fonts.h"

// ---------------------------------------------------------------------------
// Shared panel drawing helpers
// Panels use LayerUpdateProc for custom drawing. These helpers provide
// the common mobiGlas visual elements: border, corner accents, header.
// ---------------------------------------------------------------------------

// Draw the full panel chrome: border + corner accents
static inline void panel_draw_chrome(GContext *ctx, GRect bounds, GColor color) {
  draw_panel_border(ctx, bounds, color);
  draw_corner_accents(ctx, bounds, color);
}

// Draw chrome + header label (combines border, accents, title)
static inline void panel_draw_header_full(GContext *ctx, GRect bounds,
                                          const char *title, GColor color) {
  panel_draw_chrome(ctx, bounds, color);
  draw_panel_header(ctx, bounds, title, fonts_get(FONT_SIZE_HEADER), color);
}

// Content area: the usable rect below the header (below underline at y+20)
static inline GRect panel_content_rect(GRect bounds) {
  return GRect(bounds.origin.x + 4, bounds.origin.y + 20,
               bounds.size.w - 8, bounds.size.h - 24);
}
