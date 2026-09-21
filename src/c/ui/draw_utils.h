#pragma once

#include <pebble.h>
#include "../watchface.h"

// Fill panel background with a dark color
void draw_panel_fill(GContext *ctx, GRect bounds, GColor color);

// Draw a 1px stroke rectangle border
void draw_panel_border(GContext *ctx, GRect bounds, GColor color);

// Draw L-shaped corner accents (6px legs, 1px inset) at all 4 corners
void draw_corner_accents(GContext *ctx, GRect bounds, GColor color);

// Draw panel header: accent bar + label + partial underline + arrow glyph
void draw_panel_header(GContext *ctx, GRect bounds, const char *title,
                       GFont font, GColor color);

// Extended header with optional right-aligned label (NULL = title only)
void draw_panel_header_ex(GContext *ctx, GRect bounds, const char *title,
                          const char *right_label, GFont font, GColor color);

// Draw a segmented battery bar (6 segments)
void draw_battery_bar(GContext *ctx, GRect bounds, int percent, GColor color);

// Draw a single ring gauge (track + filled arc), thickness inset from edge
void draw_ring_gauge(GContext *ctx, GRect box, int percent,
                     uint16_t thickness, GColor track, GColor fill);

// Draw two concentric ring gauges (outer + inner)
void draw_dual_ring(GContext *ctx, GRect box,
                    int outer_pct, GColor outer_col,
                    int inner_pct, GColor inner_col);
