#pragma once

#include <pebble.h>

// Draw a 1px stroke rectangle border
void draw_panel_border(GContext *ctx, GRect bounds, GColor color);

// Draw L-shaped corner accents (4px legs) at all 4 corners
void draw_corner_accents(GContext *ctx, GRect bounds, GColor color);

// Draw a panel header: label text left-aligned with 1px underline
void draw_panel_header(GContext *ctx, GRect bounds, const char *title,
                       GFont font, GColor color);

// Draw a segmented battery bar (6 segments)
void draw_battery_bar(GContext *ctx, GRect bounds, int percent, GColor color);
