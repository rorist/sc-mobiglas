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
// color = title/accent/underline color; right_color = right label color
void draw_panel_header_ex(GContext *ctx, GRect bounds, const char *title,
                          const char *right_label, GFont font, GColor color,
                          GColor right_color);

// Draw a segmented battery bar (6 segments)
void draw_battery_bar(GContext *ctx, GRect bounds, int percent, GColor color);

// Draw a single ring gauge (track + filled arc), thickness inset from edge
void draw_ring_gauge(GContext *ctx, GRect box, int percent,
                     uint16_t thickness, GColor track, GColor fill);

// Draw two concentric ring gauges (outer + inner)
void draw_dual_ring(GContext *ctx, GRect box,
                    int outer_pct, GColor outer_col,
                    int inner_pct, GColor inner_col);

// Draw an 8x8px holo-style weather icon at origin (top-left)
// cond_idx: 0 CLEAR, 1 CLOUDY, 2 FOG, 3 RAIN, 4 SNOW, 5 STORM, 6 UNKNOWN
// Color by severity: CLEAR/CLOUDY/FOG/UNKNOWN = configurable LABEL color,
// RAIN/SNOW = COLOR_WARN, STORM = COLOR_ALERT
void draw_weather_icon(GContext *ctx, GPoint origin, int cond_idx);

// Draw an 8x8px holo-style sun icon at origin (top-left)
// sunset=false: sun (circle + rays), sunset=true: half moon (right arc + chord)
// Color: configurable LABEL color
void draw_sun_icon(GContext *ctx, GPoint origin, bool sunset);

// Draw an 8x8px holo-style drop (humidity) icon at origin (top-left)
// Color: configurable LABEL color
void draw_drop_icon(GContext *ctx, GPoint origin);

// Draw an 8x8px holo-style NE arrow (wind) icon at origin (top-left)
// Color: configurable LABEL color
void draw_wind_icon(GContext *ctx, GPoint origin);

// Draw an 8x8px holo-style UV icon at origin (top-left): 3 parallel beams
// Color: configurable LABEL color
void draw_uv_icon(GContext *ctx, GPoint origin);
