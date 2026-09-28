#include "fonts.h"

static GFont s_fonts[FONT_SIZE_COUNT];

void fonts_init(void) {
  s_fonts[FONT_SIZE_HEADER] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_14));
  s_fonts[FONT_SIZE_VALUE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_18));
#if PBL_DISPLAY_WIDTH < 200
  // Flint (144x168): smaller time fonts fit the narrow screen
  s_fonts[FONT_SIZE_TIME] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_40));
  s_fonts[FONT_SIZE_TIME_BIG] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_48));
  // Flint is too narrow for 72px: TIME_HUGE reuses the 48px face
  s_fonts[FONT_SIZE_TIME_HUGE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_48));
#else
  s_fonts[FONT_SIZE_TIME] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_50));
  s_fonts[FONT_SIZE_TIME_BIG] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_60));
  s_fonts[FONT_SIZE_TIME_HUGE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_72));
#endif
}

void fonts_deinit(void) {
  for (int i = 0; i < FONT_SIZE_COUNT; i++) {
    if (s_fonts[i]) {
      fonts_unload_custom_font(s_fonts[i]);
      s_fonts[i] = NULL;
    }
  }
}

GFont fonts_get(FontSize size) {
  if (size < FONT_SIZE_COUNT && s_fonts[size]) {
    return s_fonts[size];
  }
  // Fallback to system Gothic
  return fonts_get_system_font(FONT_KEY_GOTHIC_14);
}
