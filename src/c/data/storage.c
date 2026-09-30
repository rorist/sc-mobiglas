// SC mobiGlas — persistent storage (flash) for config + logo
#include "storage.h"
#include "../watchface.h"

#define PERSIST_KEY_CONFIG     1
#define PERSIST_KEY_LOGO       2
#define PERSIST_KEY_COLOR_TIME   3
#define PERSIST_KEY_COLOR_VALUE  4
#define PERSIST_KEY_COLOR_LABEL  5
#define PERSIST_KEY_COLOR_HEADER 6
#define PERSIST_KEY_COLOR_WARN   7
#define PERSIST_KEY_SHOW_DATE    8
#define PERSIST_KEY_MED_METRICS  9
#define PERSIST_KEY_ENV_METRICS  10
#define PERSIST_KEY_SYS_METRICS  11

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

// ---------------------------------------------------------------------------
// Show date toggle (1 = show, 0 = hide; default shown)
// ---------------------------------------------------------------------------

uint32_t storage_load_show_date(void) {
  if (!persist_exists(PERSIST_KEY_SHOW_DATE)) return 1;
  return (uint32_t)persist_read_int(PERSIST_KEY_SHOW_DATE);
}

void storage_save_show_date(uint32_t show) {
  persist_write_int(PERSIST_KEY_SHOW_DATE, (int32_t)show);
}

// ---------------------------------------------------------------------------
// Per-panel metrics masks (bit i = metric i enabled, fixed C-side order)
// ---------------------------------------------------------------------------

static uint32_t prv_load_mask(int key, uint32_t def) {
  if (!persist_exists(key)) return def;
  return (uint32_t)persist_read_int(key);
}

uint32_t storage_load_med_metrics(void) {
  return prv_load_mask(PERSIST_KEY_MED_METRICS, 0x03);  // BPM + STEPS
}

void storage_save_med_metrics(uint32_t mask) {
  persist_write_int(PERSIST_KEY_MED_METRICS, (int32_t)mask);
}

uint32_t storage_load_env_metrics(void) {
  return prv_load_mask(PERSIST_KEY_ENV_METRICS, 0x3F);  // all 6
}

void storage_save_env_metrics(uint32_t mask) {
  persist_write_int(PERSIST_KEY_ENV_METRICS, (int32_t)mask);
}

uint32_t storage_load_sys_metrics(void) {
  return prv_load_mask(PERSIST_KEY_SYS_METRICS, 0x03);  // BAT + COM
}

void storage_save_sys_metrics(uint32_t mask) {
  persist_write_int(PERSIST_KEY_SYS_METRICS, (int32_t)mask);
}

// ---------------------------------------------------------------------------
// Configurable text colors (persisted as GColor.argb uint8)
// ---------------------------------------------------------------------------

static GColor prv_load_color(int key, GColor def) {
  if (!persist_exists(key)) return def;
  return (GColor){ .argb = (uint8_t)persist_read_int(key) };
}

// Persist keys and defaults per ColorSlot — keys 3..7 in slot order
static const uint8_t s_color_key[COLOR_SLOT_COUNT] = {
  PERSIST_KEY_COLOR_TIME, PERSIST_KEY_COLOR_VALUE, PERSIST_KEY_COLOR_LABEL,
  PERSIST_KEY_COLOR_HEADER, PERSIST_KEY_COLOR_WARN,
};
static const GColor s_color_default[COLOR_SLOT_COUNT] = {
  COLOR_TIME_DEFAULT, COLOR_VALUE_DEFAULT, COLOR_LABEL_DEFAULT,
  COLOR_HEADER_DEFAULT, COLOR_WARN_DEFAULT,
};

GColor storage_load_color(ColorSlot slot) {
  return prv_load_color(s_color_key[slot], s_color_default[slot]);
}

void storage_save_color(ColorSlot slot, GColor color) {
  persist_write_int(s_color_key[slot], (int32_t)color.argb);
}

