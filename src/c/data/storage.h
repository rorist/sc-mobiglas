#pragma once

#include <pebble.h>

// Persistent storage (flash) — config bitmask + constructor logo.
// Values survive watch reboots.

uint32_t storage_load_config(void);
void storage_save_config(uint32_t config);

uint32_t storage_load_logo(void);
void storage_save_logo(uint32_t logo);

// Show date toggle (1 = show, 0 = hide)
uint32_t storage_load_show_date(void);
void storage_save_show_date(uint32_t show);

// Per-panel metrics masks (bit i = metric i enabled, fixed C-side order)
// Defaults: MED = BPM+STEPS (0x03), ENV = all 6 (0x3F), SYS = BAT+COM (0x03)
uint32_t storage_load_med_metrics(void);
void storage_save_med_metrics(uint32_t mask);
uint32_t storage_load_env_metrics(void);
void storage_save_env_metrics(uint32_t mask);
uint32_t storage_load_sys_metrics(void);
void storage_save_sys_metrics(uint32_t mask);

// Configurable text colors (persisted as GColor.argb)
GColor storage_load_color_time(void);
GColor storage_load_color_value(void);
GColor storage_load_color_label(void);
GColor storage_load_color_header(void);
GColor storage_load_color_warn(void);
void storage_save_color_time(GColor color);
void storage_save_color_value(GColor color);
void storage_save_color_label(GColor color);
void storage_save_color_header(GColor color);
void storage_save_color_warn(GColor color);

