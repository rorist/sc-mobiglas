#pragma once

#include <pebble.h>

// Persistent storage (flash) — config bitmask + constructor logo.
// Values survive watch reboots.

uint32_t storage_load_config(void);
void storage_save_config(uint32_t config);

uint32_t storage_load_logo(void);
void storage_save_logo(uint32_t logo);
