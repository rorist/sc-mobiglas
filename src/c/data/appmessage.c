#include "appmessage.h"
#include "storage.h"
#include "../watchface.h"
#include "../panels/environ_panel.h"
#include <stdlib.h>

// ---------------------------------------------------------------------------
// AppMessage inbox — dispatch phone data to panels
// ---------------------------------------------------------------------------

// Apply a Clay toggle (Int32 0/1) to one bit of the config bitmask.
// Returns config unchanged if the key is absent.
static uint8_t prv_apply_toggle(DictionaryIterator *iter, uint32_t key,
                                uint8_t config, uint8_t bit) {
  Tuple *t = dict_find(iter, key);
  if (!t) return config;
  return (t->value->int32) ? (uint8_t)(config | bit)
                           : (uint8_t)(config & ~bit);
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

  // Constructor logo — Clay "select" sends a CString ("0".."6")
  Tuple *logo = dict_find(iter, MESSAGE_KEY_KEY_LOGO);
  if (logo) {
    int logo_val = 0;
    if (logo->type == TUPLE_CSTRING) {
      logo_val = atoi(logo->value->cstring);
    } else {
      logo_val = (int)logo->value->int32;
    }
    if (logo_val < 0 || logo_val > 6) logo_val = 0;
    watchface_set_logo((uint8_t)logo_val);
    storage_save_logo((uint8_t)logo_val);
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
  AppMessageResult res = app_message_open(256, 256);
  if (res != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_open failed: %d", (int)res);
  }
}

