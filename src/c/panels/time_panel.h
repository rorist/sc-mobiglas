#pragma once

#include <pebble.h>

// Create the TIME panel layer at the given bounds
Layer *time_panel_create(GRect bounds);

// Destroy the TIME panel and free resources
void time_panel_destroy(void);

// Update the time display (called from tick handler)
void time_panel_update(struct tm *tick_time);
