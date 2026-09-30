#include "fonts.h"

static GFont s_fonts[FONT_SIZE_COUNT];

void fonts_init(void) {
  s_fonts[FONT_SIZE_HEADER] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_14));
#if PBL_DISPLAY_WIDTH >= 200
  // RAJDHANI_18 is only used by the date line (time_panel.c), which
  // compiles with the 14px HEADER face on narrow (flint) screens
  s_fonts[FONT_SIZE_VALUE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_18));
#endif
  s_fonts[FONT_SIZE_METRIC] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_12));
#if PBL_DISPLAY_WIDTH < 200
  // Flint (144x168): smaller time fonts fit the narrow screen
  s_fonts[FONT_SIZE_TIME] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_40));
  s_fonts[FONT_SIZE_TIME_BIG] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_48));
  // Flint is too narrow for 72/80px: TIME_HUGE/TIME_MASSIVE reuse the 48px
  // face, and TIME_SMALL (below) reuses the 40px face — one load per size,
  // aliased slots are cleared together in fonts_deinit
  s_fonts[FONT_SIZE_TIME_HUGE] = s_fonts[FONT_SIZE_TIME_BIG];
  s_fonts[FONT_SIZE_TIME_MASSIVE] = s_fonts[FONT_SIZE_TIME_BIG];
#else
  s_fonts[FONT_SIZE_TIME] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_50));
  s_fonts[FONT_SIZE_TIME_BIG] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_60));
  s_fonts[FONT_SIZE_TIME_HUGE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_72));
  s_fonts[FONT_SIZE_TIME_MASSIVE] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_80));
#endif
#if defined(PBL_ROUND)
  // Gabbro: logo mode uses the 40px face — the narrowed time zone
  // cannot fit the 50px face (see time_panel.c)
  s_fonts[FONT_SIZE_TIME_SMALL] = fonts_load_custom_font(
      resource_get_handle(RESOURCE_ID_FONT_RAJDHANI_40));
#elif PBL_DISPLAY_WIDTH < 200
  // Flint: TIME_SMALL is the same 40px face as TIME — share the handle
  s_fonts[FONT_SIZE_TIME_SMALL] = s_fonts[FONT_SIZE_TIME];
#endif
}

void fonts_deinit(void) {
  for (int i = 0; i < FONT_SIZE_COUNT; i++) {
    GFont f = s_fonts[i];
    if (!f) continue;
    fonts_unload_custom_font(f);
    // Clear every slot holding the same handle (flint aliases several
    // sizes to one face) so each font is unloaded exactly once
    for (int j = i; j < FONT_SIZE_COUNT; j++) {
      if (s_fonts[j] == f) s_fonts[j] = NULL;
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
