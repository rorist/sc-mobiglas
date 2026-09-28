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
// Configurable text colors (persisted as GColor.argb uint8)
// ---------------------------------------------------------------------------

static GColor prv_load_color(int key, GColor def) {
  if (!persist_exists(key)) return def;
  return (GColor){ .argb = (uint8_t)persist_read_int(key) };
}

GColor storage_load_color_time(void) {
  return prv_load_color(PERSIST_KEY_COLOR_TIME, COLOR_TIME_DEFAULT);
}

GColor storage_load_color_value(void) {
  return prv_load_color(PERSIST_KEY_COLOR_VALUE, COLOR_VALUE_DEFAULT);
}

GColor storage_load_color_label(void) {
  return prv_load_color(PERSIST_KEY_COLOR_LABEL, COLOR_LABEL_DEFAULT);
}

GColor storage_load_color_header(void) {
  return prv_load_color(PERSIST_KEY_COLOR_HEADER, COLOR_HEADER_DEFAULT);
}

GColor storage_load_color_warn(void) {
  return prv_load_color(PERSIST_KEY_COLOR_WARN, COLOR_WARN_DEFAULT);
}

void storage_save_color_time(GColor color) {
  persist_write_int(PERSIST_KEY_COLOR_TIME, (int32_t)color.argb);
}

void storage_save_color_value(GColor color) {
  persist_write_int(PERSIST_KEY_COLOR_VALUE, (int32_t)color.argb);
}

void storage_save_color_label(GColor color) {
  persist_write_int(PERSIST_KEY_COLOR_LABEL, (int32_t)color.argb);
}

void storage_save_color_header(GColor color) {
  persist_write_int(PERSIST_KEY_COLOR_HEADER, (int32_t)color.argb);
}

void storage_save_color_warn(GColor color) {
  persist_write_int(PERSIST_KEY_COLOR_WARN, (int32_t)color.argb);
}

