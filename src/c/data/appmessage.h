#pragma once

#include <pebble.h>

// AppMessage keys — must match package.json "messageKeys"
// (this SDK does not generate KEY_* macros, values defined here)
typedef enum {
  KEY_TEMP = 0,        // Int8   — temperature (Celsius canonical, °F per config)
  KEY_WEATHER = 1,     // CString — SUNNY / CLOUDY / RAIN / SNOW / FOG
  KEY_EVENT_TITLE = 2, // CString — next event title (max 18 chars)
  KEY_EVENT_TIME = 3,  // Int32  — next event Unix epoch
  KEY_CONFIG = 4,      // UInt8  — config bitmask (see watchface.h)
} AppMessageKey;

// Open AppMessage buffers and register handlers.
// Must be called after watchface_create() (panels exist) so incoming
// messages can be dispatched immediately.
void appmessage_init(void);

