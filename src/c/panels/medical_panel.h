#pragma once

#include <pebble.h>

Layer *medical_panel_create(GRect bounds);
void medical_panel_destroy(void);
void medical_panel_update_bounds(GRect bounds);
