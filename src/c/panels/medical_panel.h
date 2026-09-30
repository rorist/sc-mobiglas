#pragma once

#include <pebble.h>

Layer *medical_panel_create(GRect bounds);
void medical_panel_destroy(void);

// Debug health injection (CLI/emulator channel): idx 0-7, val >= 0 sets the
// override (native units: bpm, steps, seconds, meters, kcal), val < 0 clears.
void medical_panel_set_debug(int idx, int32_t val);
