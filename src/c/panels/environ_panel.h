#pragma once

#include <pebble.h>

Layer *environ_panel_create(GRect bounds);
void environ_panel_destroy(void);
void environ_panel_update_bounds(GRect bounds);

// Update weather data (called from appmessage handler)
void environ_panel_set_weather(int8_t temp_c, const char *condition);

// Update calendar event (called from appmessage handler)
void environ_panel_set_event(const char *title, uint32_t event_time);
