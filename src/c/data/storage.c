// SC mobiGlas — persistent storage (flash) for config + logo
#include "storage.h"
#include "../watchface.h"

#define PERSIST_KEY_CONFIG 1
#define PERSIST_KEY_LOGO   2

// ---------------------------------------------------------------------------
// Config bitmask
// ---------------------------------------------------------------------------

uint32_t storage_load_config(void) {
  if (!persist_exists(PERSIST_KEY_CONFIG)) return CONFIG_DEFAULT;
  return (uint32_t)persist_read_int(PERSIST_KEY_CONFIG);
}

void storage_save_config(uint32_t config) {
  persist_write_int(PERSIST_KEY_CONFIG, (int32_t)config);
}

// ---------------------------------------------------------------------------
// Constructor logo (0 = NONE, 1..6 = logo resource id)
// ---------------------------------------------------------------------------

uint32_t storage_load_logo(void) {
  if (!persist_exists(PERSIST_KEY_LOGO)) return 7;  // Star Citizen default
  return (uint32_t)persist_read_int(PERSIST_KEY_LOGO);
}

void storage_save_logo(uint32_t logo) {
  persist_write_int(PERSIST_KEY_LOGO, (int32_t)logo);
}
