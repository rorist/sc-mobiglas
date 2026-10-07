#pragma once

#include <pebble.h>

// ---------------------------------------------------------------------------
// Color palette — mapped to nearest Pebble 6-bit GColor
// ---------------------------------------------------------------------------
#define COLOR_PRIMARY    GColorVividCerulean
#define COLOR_BG         GColorBlack
// B&W (flint): black panels on black bg, structure comes from borders/accents
#define COLOR_PANEL_BG   PBL_IF_BW_ELSE(GColorBlack, GColorOxfordBlue)
// 1-bit (flint): orange/red binarize to black on black = invisible (low
// battery, STORM icon, abnormal HR, COM ERR) — clamp semantic colors to white
#define COLOR_WARN       PBL_IF_BW_ELSE(GColorWhite, GColorOrange)
#define COLOR_SAFE       PBL_IF_BW_ELSE(GColorWhite, GColorMalachite)
#define COLOR_ALERT      PBL_IF_BW_ELSE(GColorWhite, GColorRed)
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
#if PBL_DISPLAY_WIDTH < 200  // flint: tight 144x168 budget
#define MARGIN        2
#define PANEL_GAP     1
#else
#define MARGIN        PBL_IF_ROUND_ELSE(24, 3)
#define PANEL_GAP     2
#endif

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
// Flint's 168px height is tight: mins sum to the exact available height
// (168 - 2*2 margins - 2*1 gaps = 162; surplus splits across rows)
// ---------------------------------------------------------------------------
#if PBL_DISPLAY_WIDTH < 200  // flint
#define TIME_MIN_H     74  // 48px face + 14px date + chrome (block 57 of 58)
#define MEDICAL_MIN_H  56  // header + 2 compact rings
#define ENVIRON_MIN_H  56  // header + 3 data rows
#define SYSTEMS_MIN_H  32  // 14px line + chrome (74+56+56+32 = 162 = avail)
#else  // emery / gabbro
#define TIME_MIN_H     88  // 60px font + date + chrome
#define TIME_MAX_H    120  // cap with data rows visible (72px face + date
                          // = content 100); surplus beyond feeds the rows
#define MEDICAL_MIN_H  70  // header + 2 data rows
#define ENVIRON_MIN_H  70  // header + 3 data rows
#define SYSTEMS_MIN_H  38  // 14px line + 2px bottom clearance (content 16)
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

// Per-panel metrics masks (bit i = metric i enabled, fixed C-side order)
// MED bits: 0 BPM, 1 STEPS, 2 SLEEP, 3 KCAL, 4 DIST, 5 ACTIVE, 6 RKCAL, 7 DSLEEP
// ENV bits: 0 WEATHER, 1 WIND, 2 HUM, 3 UV, 4 SUNRISE, 5 SUNSET
// SYS bits: 0 BAT, 1 COM
uint32_t watchface_get_med_metrics(void);
uint32_t watchface_get_env_metrics(void);
uint32_t watchface_get_sys_metrics(void);
void watchface_set_med_metrics(uint32_t mask);
void watchface_set_env_metrics(uint32_t mask);
void watchface_set_sys_metrics(uint32_t mask);


// Configurable text colors (runtime) — ColorSlot indexes both the storage
// persist keys and the Clay color pickers, in this order.
typedef enum {
  COLOR_SLOT_TIME = 0,
  COLOR_SLOT_VALUE,
  COLOR_SLOT_LABEL,
  COLOR_SLOT_HEADER,
  COLOR_SLOT_WARN,
  COLOR_SLOT_COUNT
} ColorSlot;

// Slot-based access (clamped to white on B&W flint)
GColor watchface_get_color(ColorSlot slot);
void watchface_set_color(ColorSlot slot, GColor color);

// Named getters used by the panels
GColor watchface_get_color_time(void);
GColor watchface_get_color_value(void);
GColor watchface_get_color_label(void);
GColor watchface_get_color_header(void);
GColor watchface_get_color_warn(void);


// ---------------------------------------------------------------------------
// Watchface lifecycle
// ---------------------------------------------------------------------------
void watchface_create(Window *window);
void watchface_destroy(void);
void watchface_tick(struct tm *tick_time, TimeUnits units_changed);
void watchface_update_config(uint8_t config);
