#include "appmessage.h"
#include "../watchface.h"
#include "../panels/environ_panel.h"

// ---------------------------------------------------------------------------
// AppMessage inbox — dispatch phone data to panels
// ---------------------------------------------------------------------------

static void prv_inbox_received(DictionaryIterator *iter, void *ctx) {
  // Weather: temperature + condition (both sent together by weather.js)
  Tuple *temp = dict_find(iter, KEY_TEMP);
  Tuple *cond = dict_find(iter, KEY_WEATHER);
  if (temp && cond) {
    environ_panel_set_weather((int8_t)temp->value->int8, cond->value->cstring);
  } else if (temp || cond) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Weather message incomplete: temp=%p cond=%p",
            (void *)temp, (void *)cond);
  }

  // Calendar: next event title + Unix epoch
  Tuple *evt_title = dict_find(iter, KEY_EVENT_TITLE);
  Tuple *evt_time = dict_find(iter, KEY_EVENT_TIME);
  if (evt_title || evt_time) {
    const char *title = evt_title ? evt_title->value->cstring : NULL;
    uint32_t epoch = evt_time ? (uint32_t)evt_time->value->int32 : 0;
    environ_panel_set_event(title, epoch);
  }

  // Config bitmask — rebuild layout if changed
  Tuple *cfg = dict_find(iter, KEY_CONFIG);
  if (cfg) {
    uint8_t config = cfg->value->uint8;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Config received: 0x%02x", config);
    watchface_update_config(config);
    // TODO: persist via storage (see todo-list, data layer)
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

