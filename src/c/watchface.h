#pragma once

#include <pebble.h>

// ---------------------------------------------------------------------------
// Color palette — mapped to nearest Pebble 6-bit GColor
// ---------------------------------------------------------------------------
// #38bdf8 -> GColorVividCerulean  (0x00AAFF)
// #0ea5e9 -> GColorCobaltBlue    (0x0055AA)
// #06090f -> GColorBlack          (0x000000)
// ---------------------------------------------------------------------------
#define COLOR_PRIMARY    GColorVividCerulean
#define COLOR_SECONDARY  GColorCobaltBlue
#define COLOR_BG         GColorBlack
#define COLOR_TEXT       GColorVividCerulean

// ---------------------------------------------------------------------------
// Layout constants — Emery (PT2): 200 x 228
// ---------------------------------------------------------------------------
#define MARGIN       3
#define PANEL_GAP    2

// Full-width panel width: 200 - 2*MARGIN = 194
#define PANEL_FULL_W  194

// Half-width panel: (194 - PANEL_GAP) / 2 = 96
#define PANEL_HALF_W  96

// Panel heights
#define TIME_PANEL_H     60
#define MID_PANEL_H      98
#define SYSTEMS_PANEL_H  60

// Panel Y origins
#define TIME_PANEL_Y     MARGIN                                         // 3
#define MID_PANEL_Y      (TIME_PANEL_Y + TIME_PANEL_H + PANEL_GAP)     // 65
#define SYSTEMS_PANEL_Y  (MID_PANEL_Y + MID_PANEL_H + PANEL_GAP)       // 165

// Panel rectangles
#define RECT_TIME     GRect(MARGIN, TIME_PANEL_Y, PANEL_FULL_W, TIME_PANEL_H)
#define RECT_MEDICAL  GRect(MARGIN, MID_PANEL_Y, PANEL_HALF_W, MID_PANEL_H)
#define RECT_ENVIRON  GRect(MARGIN + PANEL_HALF_W + PANEL_GAP, MID_PANEL_Y, PANEL_HALF_W, MID_PANEL_H)
#define RECT_SYSTEMS  GRect(MARGIN, SYSTEMS_PANEL_Y, PANEL_FULL_W, SYSTEMS_PANEL_H)

// Panel IDs
#define PANEL_TIME     0
#define PANEL_MEDICAL  1
#define PANEL_ENVIRON  2
#define PANEL_SYSTEMS  3

// ---------------------------------------------------------------------------
// Watchface lifecycle
// ---------------------------------------------------------------------------
void watchface_create(Window *window);
void watchface_destroy(void);
void watchface_tick(struct tm *tick_time, TimeUnits units_changed);
