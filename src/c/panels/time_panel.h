#pragma once

#include <pebble.h>

Layer *time_panel_create(GRect bounds);
void time_panel_destroy(void);
void time_panel_update(struct tm *tick_time);
void time_panel_update_bounds(GRect bounds);
