#pragma once

#include <pebble.h>

Layer *environ_panel_create(GRect bounds);
void environ_panel_destroy(void);

// Update weather data (called from appmessage handler)
void environ_panel_set_weather(int8_t temp_c, const char *condition);

// Re-format current data for a config change (°C/°F) without new data
void environ_panel_refresh_config(void);

// Update wind (speed km/h, direction degrees; -1 = not provided)
void environ_panel_set_wind(int16_t speed_kmh, int16_t dir_deg);

// Update humidity (%, -1 = not provided) and UV index (-1 = not provided)
void environ_panel_set_humidity_uv(int8_t humidity, int8_t uv);

// Update sun times ("HH:MM" strings, NULL = not provided)
void environ_panel_set_sun(const char *sunrise, const char *sunset);
