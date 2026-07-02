#pragma once

#include <pebble.h>

// ---------------------------------------------------------------------------
// Color palette — mapped to nearest Pebble 6-bit GColor
// ---------------------------------------------------------------------------
#define COLOR_PRIMARY    GColorVividCerulean
#define COLOR_SECONDARY  GColorCobaltBlue
#define COLOR_BG         GColorBlack
#define COLOR_PANEL_BG   GColorOxfordBlue
#define COLOR_TEXT       GColorVividCerulean
#define COLOR_WARN       GColorOrange
#define COLOR_SAFE       GColorMalachite
#define COLOR_GAUGE_BG   GColorCobaltBlue

// ---------------------------------------------------------------------------
// Layout constants — Emery (PT2): 200 x 228
// ---------------------------------------------------------------------------
#define SCREEN_W      200
#define SCREEN_H      228
#define MARGIN        3
#define PANEL_GAP     2

// Full-width panel width
#define PANEL_FULL_W  (SCREEN_W - 2 * MARGIN)   // 194

// Half-width panel (for side-by-side MEDICAL/ENVIRON)
#define PANEL_HALF_W  ((PANEL_FULL_W - PANEL_GAP) / 2)  // 96

// ---------------------------------------------------------------------------
// Panel IDs (bit positions match config bitmask bits 2-5)
// ---------------------------------------------------------------------------
#define PANEL_TIME     0  // always visible
#define PANEL_MEDICAL  1  // config bit 2
#define PANEL_ENVIRON  2  // config bit 3
#define PANEL_SYSTEMS  3  // config bit 4
#define PANEL_COUNT    4

// ---------------------------------------------------------------------------
// Panel minimum heights — used by layout engine
// ---------------------------------------------------------------------------
#define TIME_MIN_H     78  // 48px font + date + chrome
#define MEDICAL_MIN_H  70  // header + 2 data rows
#define ENVIRON_MIN_H  70  // header + 3 data rows
#define SYSTEMS_MIN_H  44  // header + battery bar

// ---------------------------------------------------------------------------
// Config bitmask (from KEY_CONFIG AppMessage)
// ---------------------------------------------------------------------------
#define CONFIG_12H       (1 << 0)
#define CONFIG_FAHRENHEIT (1 << 1)
#define CONFIG_MEDICAL   (1 << 2)
#define CONFIG_ENVIRON   (1 << 3)
#define CONFIG_SYSTEMS   (1 << 4)
#define CONFIG_SECONDS   (1 << 5)

// Default config: all panels ON, 24h, Celsius
#define CONFIG_DEFAULT   (CONFIG_MEDICAL | CONFIG_ENVIRON | CONFIG_SYSTEMS)

// ---------------------------------------------------------------------------
// Layout result — computed at runtime by layout engine
// ---------------------------------------------------------------------------
typedef struct {
  bool     visible[PANEL_COUNT];
  GRect    rects[PANEL_COUNT];
  int      panel_count;  // number of visible panels
} LayoutInfo;

// Compute layout for the given config bitmask
// screen_bounds: the full window bounds (200x228 on emery)
LayoutInfo layout_compute(GRect screen_bounds, uint8_t config);

// ---------------------------------------------------------------------------
// Watchface lifecycle
// ---------------------------------------------------------------------------
void watchface_create(Window *window);
void watchface_destroy(void);
void watchface_tick(struct tm *tick_time, TimeUnits units_changed);
void watchface_update_config(uint8_t config);
