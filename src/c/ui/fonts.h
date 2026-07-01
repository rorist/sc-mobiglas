#pragma once

#include <pebble.h>

// Font size identifiers
typedef enum {
  FONT_SIZE_HEADER = 0,  // 14px — panel headers, date, small text
  FONT_SIZE_VALUE,       // 18px — panel values (HR, temp, etc.)
  FONT_SIZE_TIME,        // 48px — HH:MM large time display
  FONT_SIZE_COUNT
} FontSize;

// Load all custom fonts from resources
void fonts_init(void);

// Unload all custom fonts
void fonts_deinit(void);

// Get font by size enum
GFont fonts_get(FontSize size);
