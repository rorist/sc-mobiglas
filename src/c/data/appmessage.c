#include "appmessage.h"
#include "storage.h"
#include "../watchface.h"
#include "../panels/environ_panel.h"
#include <stdlib.h>

// ---------------------------------------------------------------------------
// AppMessage inbox — dispatch phone data to panels
// ---------------------------------------------------------------------------

// Parse a Clay color CString "#rrggbb" to a GColor (nearest 64-color match)
// NOTE: no sscanf — Pebble libc has no full stdio (link errors)
static GColor prv_parse_color(const char *hex) {
  if (!hex || hex[0] != '#') return GColorVividCerulean;
  unsigned int v = 0;
  for (int i = 1; i <= 6 && hex[i]; i++) {
    char c = hex[i];
    int d;
    if (c >= '0' && c <= '9') d = c - '0';
    else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
    else return GColorVividCerulean;
    v = (v << 4) | (unsigned int)d;
  }
  return GColorFromRGBA((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, 255);
}

// Clay "color" sends the value as an Int32 (0xRRGGBB packed by the picker);
// tolerate a CString "#rrggbb" as well.
static GColor prv_color_from_tuple(Tuple *t) {
  if (!t) return GColorVividCerulean;
  if (t->type == TUPLE_CSTRING) return prv_parse_color(t->value->cstring);
  unsigned int v = (unsigned int)t->value->int32;
  return GColorFromRGBA((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, 255);
}

// Apply a Clay toggle (Int32 0/1) to one bit of the config bitmask.
// Returns config unchanged if the key is absent.
static uint8_t prv_apply_toggle(DictionaryIterator *iter, uint32_t key,
                                uint8_t config, uint8_t bit) {
  Tuple *t = dict_find(iter, key);
  if (!t) return config;
  return (t->value->int32) ? (uint8_t)(config | bit)
                           : (uint8_t)(config & ~bit);
}

// Merge a Clay checkboxgroup (one Int32 0/1 per item at consecutive keys)
// into the persisted mask: absent items keep their previous state.
static uint32_t prv_merge_mask(DictionaryIterator *iter, uint32_t base_key,
                               int count, uint32_t old_mask) {
  uint32_t merged = 0;
  for (int i = 0; i < count; i++) {
    Tuple *t = dict_find(iter, base_key + i);
    if (t) {
      if (t->value->int32) merged |= (1u << i);
    } else {
      merged |= (old_mask & (1u << i));
    }
  }
  return merged;
}

static void prv_inbox_received(DictionaryIterator *iter, void *ctx) {
  // Weather: temperature + condition (both sent together by weather.js)
  Tuple *temp = dict_find(iter, MESSAGE_KEY_KEY_TEMP);
  Tuple *cond = dict_find(iter, MESSAGE_KEY_KEY_WEATHER);
  if (temp && cond) {
    environ_panel_set_weather((int8_t)temp->value->int8, cond->value->cstring);
  } else if (temp || cond) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather message incomplete: temp=%p cond=%p",
            (void *)temp, (void *)cond);
  }

  // Wind: speed (Int16 km/h) + direction (Int16 degrees)
  Tuple *wind_spd = dict_find(iter, MESSAGE_KEY_KEY_WIND_SPEED);
  Tuple *wind_dir = dict_find(iter, MESSAGE_KEY_KEY_WIND_DIR);
  if (wind_spd || wind_dir) {
    environ_panel_set_wind(
        wind_spd ? (int16_t)wind_spd->value->int16 : (int16_t)-1,
        wind_dir ? (int16_t)wind_dir->value->int16 : (int16_t)-1);
  }

  // Humidity (Int8 %) + UV index (Int8)
  Tuple *hum = dict_find(iter, MESSAGE_KEY_KEY_HUMIDITY);
  Tuple *uv = dict_find(iter, MESSAGE_KEY_KEY_UV);
  if (hum || uv) {
    environ_panel_set_humidity_uv(
        hum ? (int8_t)hum->value->int8 : (int8_t)-1,
        uv ? (int8_t)uv->value->int8 : (int8_t)-1);
  }

  // Sun times (CString "HH:MM", formatted phone-side)
  Tuple *rise = dict_find(iter, MESSAGE_KEY_KEY_SUNRISE);
  Tuple *set = dict_find(iter, MESSAGE_KEY_KEY_SUNSET);
  if (rise || set) {
    environ_panel_set_sun(
        rise ? rise->value->cstring : NULL,
        set ? set->value->cstring : NULL);
  }

  // Constructor logo — Clay "select" sends a CString ("0".."9")
  Tuple *logo = dict_find(iter, MESSAGE_KEY_KEY_LOGO);
  if (logo) {
    int logo_val = 0;
    if (logo->type == TUPLE_CSTRING) {
      logo_val = atoi(logo->value->cstring);
    } else {
      logo_val = (int)logo->value->int32;
    }
    if (logo_val < 0 || logo_val > 9) logo_val = 0;
    watchface_set_logo((uint8_t)logo_val);
    storage_save_logo((uint8_t)logo_val);
  }

  // Configurable text colors — Clay "color" sends Int32 (0xRRGGBB packed);
  // prv_color_from_tuple also tolerates a CString "#rrggbb"
    const struct {
    uint32_t key;
    void (*set)(GColor);
    void (*save)(GColor);
  } color_keys[] = {
    { MESSAGE_KEY_KEY_COLOR_TIME, watchface_set_color_time,
      storage_save_color_time },
    { MESSAGE_KEY_KEY_COLOR_VALUE, watchface_set_color_value,
      storage_save_color_value },
    { MESSAGE_KEY_KEY_COLOR_LABEL, watchface_set_color_label,
      storage_save_color_label },
    { MESSAGE_KEY_KEY_COLOR_HEADER, watchface_set_color_header,
      storage_save_color_header },
    { MESSAGE_KEY_KEY_COLOR_WARN, watchface_set_color_warn,
      storage_save_color_warn },
  };
  for (unsigned i = 0; i < sizeof(color_keys) / sizeof(color_keys[0]); i++) {
    Tuple *t = dict_find(iter, color_keys[i].key);
    if (t) {
      GColor c = prv_color_from_tuple(t);
      color_keys[i].set(c);
      color_keys[i].save(c);
    }
  }

  // Show date toggle (Int32 1/0) — hides the date line, frees space for time
  Tuple *show_date = dict_find(iter, MESSAGE_KEY_KEY_SHOW_DATE);
  if (show_date) {
    bool show = show_date->value->int32 != 0;
    watchface_set_show_date(show);
    storage_save_show_date(show ? 1 : 0);
  }

  // Per-panel metrics masks — Clay "checkboxgroup" sends one Int32 (0/1) per
  // item at consecutive keys (KEY_X_METRICS + i)
    uint32_t med_mask = watchface_get_med_metrics();
  uint32_t new_med = prv_merge_mask(iter, MESSAGE_KEY_KEY_MED_METRICS, 8,
                                    med_mask);
  if (new_med != med_mask) {
    watchface_set_med_metrics(new_med);
    storage_save_med_metrics(new_med);
  }

  uint32_t env_mask = watchface_get_env_metrics();
  uint32_t new_env = prv_merge_mask(iter, MESSAGE_KEY_KEY_ENV_METRICS, 6,
                                    env_mask);
  if (new_env != env_mask) {
    watchface_set_env_metrics(new_env);
    storage_save_env_metrics(new_env);
  }

  uint32_t sys_mask = watchface_get_sys_metrics();
  uint32_t new_sys = prv_merge_mask(iter, MESSAGE_KEY_KEY_SYS_METRICS, 2,
                                    sys_mask);
  if (new_sys != sys_mask) {
    watchface_set_sys_metrics(new_sys);
    storage_save_sys_metrics(new_sys);
  }

  // Config — Clay sends each toggle as Int32 (1/0); rebuild the bitmask
  uint8_t config = watchface_get_config();
  uint8_t new_config = config;
  new_config = prv_apply_toggle(iter, MESSAGE_KEY_KEY_12H, new_config, CONFIG_12H);
  new_config = prv_apply_toggle(iter, MESSAGE_KEY_KEY_FAHRENHEIT, new_config, CONFIG_FAHRENHEIT);
  new_config = prv_apply_toggle(iter, MESSAGE_KEY_KEY_SHOW_MEDICAL, new_config, CONFIG_MEDICAL);
  new_config = prv_apply_toggle(iter, MESSAGE_KEY_KEY_SHOW_ENVIRON, new_config, CONFIG_ENVIRON);
  new_config = prv_apply_toggle(iter, MESSAGE_KEY_KEY_SHOW_SYSTEMS, new_config, CONFIG_SYSTEMS);
  if (new_config != config) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Config: 0x%02x -> 0x%02x", config, new_config);
    watchface_update_config(new_config);
    storage_save_config(new_config);
  }
}

static void prv_inbox_dropped(AppMessageResult reason, void *ctx) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Message dropped: %d", (int)reason);
}

static void prv_outbox_sent(DictionaryIterator *iter, void *ctx) {
  // No outbox data in V1
}

static void prv_outbox_failed(DictionaryIterator *iter, AppMessageResult reason,
                              void *ctx) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox failed: %d", (int)reason);
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void appmessage_init(void) {
  app_message_register_inbox_received(prv_inbox_received);
  app_message_register_inbox_dropped(prv_inbox_dropped);
  app_message_register_outbox_sent(prv_outbox_sent);
  app_message_register_outbox_failed(prv_outbox_failed);
  // Inbox 512: full Clay payload (toggle metrics + colors) can exceed 256B;
  // outbox 64 is enough for the 1-byte weather request.
  AppMessageResult res = app_message_open(512, 64);
  if (res != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open failed: %d", (int)res);
  }
}

// Send a weather refresh request to the phone (watch-driven, tick % 30 min).
void appmessage_request_weather(void) {
  DictionaryIterator *iter;
  AppMessageResult res = app_message_outbox_begin(&iter);
  if (res != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox begin failed: %d", (int)res);
    return;
  }
  dict_write_uint8(iter, MESSAGE_KEY_KEY_REQUEST_WEATHER, 1);
  res = app_message_outbox_send();
  if (res != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox send failed: %d", (int)res);
  }
}

