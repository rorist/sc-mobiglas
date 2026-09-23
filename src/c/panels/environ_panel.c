#include "environ_panel.h"
#include "panel.h"
#include "../ui/draw_utils.h"
#include <string.h>

static Layer *s_layer;
static char s_temp_buf[12];     // "-12°C" or "--°C"
static char s_cond_buf[12];     // "CLOUDY" or "---"
static char s_wind_buf[16];     // "12 km/h WSW"
static char s_hum_buf[8];       // "68%"
static char s_uv_buf[8];        // "UV 11"

// Last known temperature in canonical Celsius; sentinel = never received
#define TEMP_UNAVAILABLE ((int8_t)-128)
static int8_t s_last_temp_c = TEMP_UNAVAILABLE;

// -1 = not received yet
static int16_t s_wind_speed = -1;
static int16_t s_wind_dir = -1;
static int8_t s_humidity = -1;
static int8_t s_uv = -1;
static bool s_sun_valid = false;

static char s_sun_rise[6] = "--:--";
static char s_sun_set[6]  = "--:--";

// Format s_temp_buf from s_last_temp_c per current config (°C/°F)
static void prv_format_temp(void) {
  if (s_last_temp_c == TEMP_UNAVAILABLE) {
    snprintf(s_temp_buf, sizeof(s_temp_buf),
             (watchface_get_config() & CONFIG_FAHRENHEIT) ? "--\u00b0F" : "--\u00b0C");
  } else if (watchface_get_config() & CONFIG_FAHRENHEIT) {
    int f = (int)s_last_temp_c * 9 / 5 + 32;
    snprintf(s_temp_buf, sizeof(s_temp_buf), "%d\u00b0F", f);
  } else {
    snprintf(s_temp_buf, sizeof(s_temp_buf), "%d\u00b0C", (int)s_last_temp_c);
  }
}

static void prv_format_wind(void) {
  if (s_wind_speed < 0) {
    snprintf(s_wind_buf, sizeof(s_wind_buf), "---");
    return;
  }
  int n = snprintf(s_wind_buf, sizeof(s_wind_buf), "%d km/h", (int)s_wind_speed);
  if (s_wind_dir >= 0 && n > 0 && n < (int)sizeof(s_wind_buf) - 5) {
    static const char *dirs[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    snprintf(s_wind_buf + n, sizeof(s_wind_buf) - n, " %s",
             dirs[((s_wind_dir + 22) / 45) % 8]);
  }
}

static void prv_format_humuv(void) {
  if (s_humidity < 0) {
    snprintf(s_hum_buf, sizeof(s_hum_buf), "---");
  } else {
    snprintf(s_hum_buf, sizeof(s_hum_buf), "%d%%", (int)s_humidity);
  }
  if (s_uv < 0) {
    snprintf(s_uv_buf, sizeof(s_uv_buf), "---");
  } else {
    snprintf(s_uv_buf, sizeof(s_uv_buf), "UV %d", (int)s_uv);
  }
}

static void prv_format_sun(void) {
  if (!s_sun_valid) {
    snprintf(s_sun_rise, sizeof(s_sun_rise), "--:--");
    snprintf(s_sun_set, sizeof(s_sun_set), "--:--");
  }
}

static int prv_cond_index(const char *c) {
  if (!c) return 6;
  if (strcmp(c, "CLEAR") == 0) return 0;
  if (strcmp(c, "CLOUDY") == 0) return 1;
  if (strcmp(c, "FOG") == 0) return 2;
  if (strcmp(c, "RAIN") == 0) return 3;
  if (strcmp(c, "SNOW") == 0) return 4;
  if (strcmp(c, "STORM") == 0) return 5;
  return 6;
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "ENVIRON", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  int x = content.origin.x;
  int y = content.origin.y;
  int w = content.size.w;
  int ch = content.size.h;

  // All ENVIRON lines are measured values, not labels
  graphics_context_set_text_color(ctx, watchface_get_color_value());

  const int line_h = 14;
  const int gap = 1;
  const int block_h = 4 * line_h + 3 * gap;
  int y0 = y + (ch - block_h) / 2;
  if (y0 < y) y0 = y;

  const int l14 = FONT_LEADING_14;

  // Line 1: icon + condition (left) + temperature (right)
  draw_weather_icon(ctx, GPoint(x, y0 + 2), prv_cond_index(s_cond_buf));
  GRect cond_rect = GRect(x + 10, y0 - l14, w - 10 - 32, line_h + l14);
  graphics_draw_text(ctx, s_cond_buf, fonts_get(FONT_SIZE_HEADER), cond_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  GRect temp_rect = GRect(x + w - 32, y0 - l14, 32, line_h + l14);
  graphics_draw_text(ctx, s_temp_buf, fonts_get(FONT_SIZE_HEADER), temp_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentRight, NULL);

  // Line 2: wind icon + wind text
  draw_wind_icon(ctx, GPoint(x, y0 + 15 + 3));
  GRect wind_rect = GRect(x + 10, y0 + 15 - l14, w - 10, line_h + l14);
  graphics_draw_text(ctx, s_wind_buf, fonts_get(FONT_SIZE_HEADER), wind_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Line 3: humidity (left half) + UV (right half), mirror of line 4
  int half = w / 2;
  draw_drop_icon(ctx, GPoint(x, y0 + 30 + 3));
  GRect hum_rect = GRect(x + 10, y0 + 30 - l14, half - 10, line_h + l14);
  graphics_draw_text(ctx, s_hum_buf, fonts_get(FONT_SIZE_HEADER), hum_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  draw_uv_icon(ctx, GPoint(x + half, y0 + 30 + 3));
  GRect uv_rect = GRect(x + half + 10, y0 + 30 - l14, w - half - 10, line_h + l14);
  graphics_draw_text(ctx, s_uv_buf, fonts_get(FONT_SIZE_HEADER), uv_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Line 4: sun icons + times — rise group left, set group right
  const int y4 = y0 + 45;
  draw_sun_icon(ctx, GPoint(x, y4 + 3), false);
  GRect rise_rect = GRect(x + 10, y4 - l14, half - 10, line_h + l14);
  graphics_draw_text(ctx, s_sun_rise, fonts_get(FONT_SIZE_HEADER), rise_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  draw_sun_icon(ctx, GPoint(x + half, y4 + 3), true);
  GRect set_rect = GRect(x + half + 10, y4 - l14, w - half - 10, line_h + l14);
  graphics_draw_text(ctx, s_sun_set, fonts_get(FONT_SIZE_HEADER), set_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
}

Layer *environ_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);

  // Keep s_last_temp_c across rebuilds so config changes re-display last data
  prv_format_temp();
  snprintf(s_cond_buf, sizeof(s_cond_buf), "---");
  prv_format_wind();
  prv_format_humuv();
  prv_format_sun();

  return s_layer;
}

void environ_panel_destroy(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void environ_panel_set_weather(int8_t temp_c, const char *condition) {
  s_last_temp_c = temp_c;
  prv_format_temp();
  if (condition) {
    strncpy(s_cond_buf, condition, sizeof(s_cond_buf) - 1);
    s_cond_buf[sizeof(s_cond_buf) - 1] = '\0';
  }
  if (s_layer) layer_mark_dirty(s_layer);
}

void environ_panel_refresh_config(void) {
  prv_format_temp();
  if (s_layer) layer_mark_dirty(s_layer);
}

void environ_panel_set_wind(int16_t speed_kmh, int16_t dir_deg) {
  if (speed_kmh >= 0) s_wind_speed = speed_kmh;
  if (dir_deg >= 0) s_wind_dir = dir_deg;
  prv_format_wind();
  if (s_layer) layer_mark_dirty(s_layer);
}

void environ_panel_set_humidity_uv(int8_t humidity, int8_t uv) {
  if (humidity >= 0) s_humidity = humidity;
  if (uv >= 0) s_uv = uv;
  prv_format_humuv();
  if (s_layer) layer_mark_dirty(s_layer);
}

void environ_panel_set_sun(const char *sunrise, const char *sunset) {
  if (sunrise && strlen(sunrise) == 5) strncpy(s_sun_rise, sunrise, 5);
  if (sunset && strlen(sunset) == 5) strncpy(s_sun_set, sunset, 5);
  if (sunrise || sunset) s_sun_valid = true;
  if (s_layer) layer_mark_dirty(s_layer);
}
