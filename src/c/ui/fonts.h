#pragma once

#include <pebble.h>

// Font size identifiers
typedef enum {
  FONT_SIZE_HEADER = 0,  // 14px — panel headers, date, small text
  FONT_SIZE_VALUE,       // 18px — panel values (HR, temp, etc.)
  FONT_SIZE_TIME,        // 48px — HH:MM large time display
  FONT_SIZE_TIME_BIG,    // 56px — HH:MM extra-large (when space allows)
  FONT_SIZE_COUNT
} FontSize;

// Load all custom fonts from resources
void fonts_init(void);

// Unload all custom fonts
void fonts_deinit(void);

// Get font by size enum
GFont fonts_get(FontSize size);

// Rajdhani leading compensation (px) — shift text rects UP by this amount
// so rendered glyphs align with the intended visual top of the rect
// (large fonts reserve empty space above glyphs inside their line box).
// V1 estimates — adjust via emulator screenshots if glyphs sit low/high.
#define FONT_LEADING_14  2
#define FONT_LEADING_18  3
#define FONT_LEADING_48  7
#define FONT_LEADING_56  8
