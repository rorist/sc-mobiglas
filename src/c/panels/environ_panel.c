#include "environ_panel.h"
#include "panel.h"
#include "../ui/draw_utils.h"
#include <string.h>

static Layer *s_layer;
static char s_temp_buf[12];     // "-12°C" or "--°C"
static char s_cond_buf[12];     // "CLOUDY" or "---"
static char s_wind_buf[16];     // "12 km/h WSW"
static char s_wind_short[12];   // "12km/h" (narrow cells, no direction)
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
    snprintf(s_wind_short, sizeof(s_wind_short), "---");
  } else {
    snprintf(s_wind_short, sizeof(s_wind_short), "%dkm/h", (int)s_wind_speed);
  }
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

// Draw one metric cell (icon + text) inside [cell_x, cell_w] on line y_line.
// Full-width metrics get the whole row; half metrics get their half.
static void prv_draw_metric(GContext *ctx, int metric, int cell_x, int cell_w,
                            int y_line) {
  const int l14 = FONT_LEADING_14;
  switch (metric) {
    case 0:  // Weather: icon + condition (left) + temperature (right)
      draw_weather_icon(ctx, GPoint(cell_x, y_line + 2),
                        prv_cond_index(s_cond_buf));
      if (cell_w >= 88) {  // narrower cells: condition glyph in icon only
        graphics_draw_text(ctx, s_cond_buf, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + 10, y_line - l14, cell_w - 10 - 32,
                                 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      // Very narrow cells (demoted weather): shrink the temp rect so it
      // doesn't slide under the icon
      const int temp_w = (cell_w < 44) ? cell_w - 12 : 32;
      graphics_draw_text(ctx, s_temp_buf, fonts_get(FONT_SIZE_HEADER),
                         GRect(cell_x + cell_w - temp_w, y_line - l14, temp_w,
                               14 + l14),
                         GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentRight, NULL);
      break;
    case 1:  // Wind
      {
        const char *wind_txt = (cell_w < 70) ? s_wind_short : s_wind_buf;
        draw_wind_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, wind_txt, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + 10, y_line - l14, cell_w - 10, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 2:  // Humidity
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_drop_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, s_hum_buf, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0), 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 3:  // UV
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_uv_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, s_uv_buf, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0), 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 4:  // Sunrise
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), false);
        graphics_draw_text(ctx, s_sun_rise, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0), 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 5:  // Sunset
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), true);
        graphics_draw_text(ctx, s_sun_set, fonts_get(FONT_SIZE_HEADER),
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0), 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
  }
}

// Metrics: 0 WEATHER (full), 1 WIND (full), 2 HUM (half), 3 UV (half),
// 4 SUNRISE (half), 5 SUNSET (half). Full metrics take their own line; two
// consecutive halves share a line; a lone trailing half is centered.
static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "ENVIRON", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  const int x = content.origin.x;
  const int y = content.origin.y;
  const int w = content.size.w;
  const int ch = content.size.h;
  const uint32_t mask = watchface_get_env_metrics();

  // All ENVIRON lines are measured values, not labels
  graphics_context_set_text_color(ctx, watchface_get_color_value());

  // Collect active metrics (fixed order). Flint side-by-side (w < 100 on a
  // 144px display): sunrise/sunset can't fit even icon-less — dropped.
  const bool narrow = (PBL_DISPLAY_WIDTH < 200) && (w < 100);
  int act[6];
  int n = 0;
  for (int i = 0; i < 6; i++) {
    if (narrow && (i == 4 || i == 5)) continue;
    if (mask & (1u << i)) act[n++] = i;
  }

  // Pack into rows: full = own row; halves pair up; lone trailing half
  // centered. If the block overflows the content height (round displays run
  // tight), a second pass demotes the full-width metrics (weather, wind) to
  // half cells sharing one line — frees a whole 15px row. Rect displays keep
  // their validated layout (narrow halves would truncate the temperature).
  const int half = w / 2;
  int rows[6][2];            // {metric, cell_x}; metric -1 = none
  int row_w[6];              // cell width for the row's first cell
  int row_count = 0;
  bool demoted = false;
  for (int pass = 0; pass < 2; pass++) {
    row_count = 0;
    demoted = (pass > 0) && PBL_IF_ROUND_ELSE(1, 0);
    for (int i = 0; i < n; ) {
      if ((act[i] <= 1) && !demoted) {      // full-width metric
        rows[row_count][0] = act[i];
        rows[row_count][1] = -1;
        row_w[row_count] = w;
        row_count++;
        i++;
      } else {                              // half metric (or demoted full)
        rows[row_count][0] = act[i];
        rows[row_count][1] = -1;
        row_w[row_count] = half;
        if (i + 1 < n && (demoted || act[i + 1] > 1)) {  // pair with next
          rows[row_count][1] = act[i + 1];
          i += 2;
        } else {
          i++;                              // lone trailing half — centered
        }
        row_count++;
      }
    }
    const int block = row_count * 14 + (row_count - 1);
    if (block <= ch) break;                 // fits
  }

  // Vertical block: one 14px line per row, 1px gaps, centered in content
  const int line_h = 14;
  const int gap = 1;
  const int block_h = row_count * line_h + (row_count - 1) * gap;
  int y0 = y + (ch - block_h) / 2;
  if (y0 < y) y0 = y;

  for (int r = 0; r < row_count; r++) {
    int y_line = y0 + r * (line_h + gap);
    int cell_x = x;
    if (rows[r][0] > 1 && rows[r][1] < 0) {
      cell_x = x + (w - half) / 2;        // lone half — centered
    }
    prv_draw_metric(ctx, rows[r][0], cell_x, row_w[r], y_line);
    if (rows[r][1] >= 0) {
      prv_draw_metric(ctx, rows[r][1], x + half, w - half, y_line);
    }
  }
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
