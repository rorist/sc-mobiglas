#pragma once

#include <pebble.h>

// ---------------------------------------------------------------------------
// Color palette — mapped to nearest Pebble 6-bit GColor
// ---------------------------------------------------------------------------
#define COLOR_PRIMARY    GColorVividCerulean
#define COLOR_BG         GColorBlack
// B&W (flint): black panels on black bg, structure comes from borders/accents
#define COLOR_PANEL_BG   PBL_IF_BW_ELSE(GColorBlack, GColorOxfordBlue)
#define COLOR_WARN       GColorOrange
#define COLOR_SAFE       GColorMalachite
#define COLOR_ALERT      GColorRed
#define COLOR_GAUGE_BG   GColorCobaltBlue

// Configurable text colors (runtime, set via settings) — see getters below
#define COLOR_TIME_DEFAULT   GColorCeleste
#define COLOR_VALUE_DEFAULT  GColorWhite
#define COLOR_LABEL_DEFAULT  GColorPictonBlue
#define COLOR_HEADER_DEFAULT GColorVividCerulean
#define COLOR_WARN_DEFAULT   GColorOrange

// ---------------------------------------------------------------------------
// Layout constants — platform-aware
// emery 200x228 (color), flint 144x168 (B&W), gabbro 260x260 (round)
// Panel widths are derived at runtime from the screen bounds (layout.c);
// only min heights and margins are fixed here.
// ---------------------------------------------------------------------------
#define MARGIN        PBL_IF_ROUND_ELSE(24, 3)
#define PANEL_GAP     2

// ---------------------------------------------------------------------------
// Panel IDs (bit positions match config bitmask bits 2-4)
// ---------------------------------------------------------------------------
#define PANEL_TIME     0  // always visible
#define PANEL_MEDICAL  1  // config bit 2
#define PANEL_ENVIRON  2  // config bit 3
#define PANEL_SYSTEMS  3  // config bit 4
#define PANEL_COUNT    4

// ---------------------------------------------------------------------------
// Panel minimum heights — used by layout engine
// Flint's 168px height is tight: shrink rows so all panels fit (sum <= 152)
// ---------------------------------------------------------------------------
#if PBL_DISPLAY_WIDTH < 200  // flint
#define TIME_MIN_H     70  // 48px font + date + chrome
#define MEDICAL_MIN_H  58  // header + 2 data rows
#define ENVIRON_MIN_H  58  // header + 3 data rows
#define SYSTEMS_MIN_H  24   // compact header (BAT inline) + battery bar
#else  // emery / gabbro
#define TIME_MIN_H     88  // 60px font + date + chrome
#define MEDICAL_MIN_H  70  // header + 2 data rows
#define ENVIRON_MIN_H  70  // header + 3 data rows
#define SYSTEMS_MIN_H  32  // compact header (BAT inline) + battery bar
#endif

// ---------------------------------------------------------------------------
// Config bitmask (rebuilt from individual Clay toggles via AppMessage)
// ---------------------------------------------------------------------------
#define CONFIG_12H       (1 << 0)
#define CONFIG_FAHRENHEIT (1 << 1)
#define CONFIG_MEDICAL   (1 << 2)
#define CONFIG_ENVIRON   (1 << 3)
#define CONFIG_SYSTEMS   (1 << 4)

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
// screen_bounds: the full window bounds (any platform)
LayoutInfo layout_compute(GRect screen_bounds, uint8_t config);

// Current active config bitmask (source of truth kept in watchface.c)
uint8_t watchface_get_config(void);

// Constructor logo displayed right of the time (0 = none, 1-9 = logo id)
uint8_t watchface_get_logo(void);
void watchface_set_logo(uint8_t logo);

// Date line visibility (hidden date frees space -> bigger time font)
bool watchface_get_show_date(void);
void watchface_set_show_date(bool show);

// Configurable text colors (runtime)
GColor watchface_get_color_time(void);
GColor watchface_get_color_value(void);
GColor watchface_get_color_label(void);
GColor watchface_get_color_header(void);
GColor watchface_get_color_warn(void);
void watchface_set_color_time(GColor color);
void watchface_set_color_value(GColor color);
void watchface_set_color_label(GColor color);
void watchface_set_color_header(GColor color);
void watchface_set_color_warn(GColor color);


// ---------------------------------------------------------------------------
// Watchface lifecycle
// ---------------------------------------------------------------------------
void watchface_create(Window *window);
void watchface_destroy(void);
void watchface_tick(struct tm *tick_time, TimeUnits units_changed);
void watchface_update_config(uint8_t config);
