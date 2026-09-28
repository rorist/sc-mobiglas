#pragma once

#include <pebble.h>

// Font size identifiers
typedef enum {
  FONT_SIZE_HEADER = 0,  // 14px — panel headers, small text
  FONT_SIZE_VALUE,       // 18px — panel values (HR, temp, etc.)
  FONT_SIZE_TIME,        // 50px — HH:MM large time display (with logo)
  FONT_SIZE_TIME_BIG,    // 60px — HH:MM extra-large (no logo)
  FONT_SIZE_TIME_HUGE,   // 72px — very tall TIME panel (no logo); flint: 48px
  FONT_SIZE_TIME_MASSIVE, // 80px — time-only hero mode / date hidden; flint: 48px
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
// Time fonts differ by platform: flint (144px wide) uses 40/48px,
// emery/gabbro use 50/60px — leading values match each pair.
#if PBL_DISPLAY_WIDTH < 200
#define FONT_LEADING_50  6
#define FONT_LEADING_60  7
#define FONT_LEADING_72  7   // flint maps TIME_HUGE to the 48px face
#define FONT_LEADING_80  7   // flint maps TIME_MASSIVE to the 48px face
#else
#define FONT_LEADING_50  7
#define FONT_LEADING_60  9
#define FONT_LEADING_72  11
#define FONT_LEADING_80  12
#endif
