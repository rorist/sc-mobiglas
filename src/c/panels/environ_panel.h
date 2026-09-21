#pragma once

#include <pebble.h>

Layer *environ_panel_create(GRect bounds);
void environ_panel_destroy(void);
void environ_panel_update_bounds(GRect bounds);

// Update weather data (called from appmessage handler)
void environ_panel_set_weather(int8_t temp_c, const char *condition);

// Re-format current data for a config change (°C/°F) without new data
void environ_panel_refresh_config(void);

// Update calendar event (called from appmessage handler)
void environ_panel_set_event(const char *title, uint32_t event_time);
