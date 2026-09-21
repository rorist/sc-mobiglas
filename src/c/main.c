#include <pebble.h>
#include "watchface.h"
#include "data/appmessage.h"

static Window *s_window;

static void prv_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  watchface_tick(tick_time, units_changed);
}

static void prv_window_load(Window *window) {
  window_set_background_color(window, COLOR_BG);
  watchface_create(window);
}

static void prv_window_unload(Window *window) {
  watchface_destroy();
}

static void prv_init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, prv_tick_handler);

  // After window load: panels exist, AppMessage data can be dispatched
  appmessage_init();
}

static void prv_deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
