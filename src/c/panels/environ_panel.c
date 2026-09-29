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
// Full-width metrics get the whole row; half metrics get their half. pad =
// extra right inset (3px: left cell of a pair must not touch the next
// icon; 0px: right/last cell, nothing follows). font/l: the metric face
// (14px, or 12px when the panel runs tight) and its leading compensation.
static void prv_draw_metric(GContext *ctx, int metric, int cell_x, int cell_w,
                            int y_line, int pad, GFont font, int l) {
  const int l14 = l;
  switch (metric) {
    case 0: {  // Weather: icon + condition + temperature flowing left
      // Temperature is measured and always shown, flowing right after the
      // condition (+4px) — no more right-aligned hole in wide cells
      const GSize temp_size = graphics_text_layout_get_content_size(
          s_temp_buf, font, GRect(cell_x, y_line - l14, cell_w, 14 + l14),
          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int temp_w = temp_size.w;
      if (temp_w > cell_w) temp_w = cell_w;
      // Icon only when the full temp still fits after it — ultra-narrow
      // shared cells fall back to temp-only text
      const bool wico = (cell_w >= 10 + temp_w);
      const int ix = cell_x + (wico ? 10 : 0);
      if (wico) {
        draw_weather_icon(ctx, GPoint(cell_x, y_line + 2),
                          prv_cond_index(s_cond_buf));
      }
      int tx = ix;  // default: right after the icon (or cell start)
      // Condition: full text if it fits before the temp, short form
      // (CLR/CLD/FOG/RN/SNW/STM) otherwise, icon only as a last resort —
      // the condition now shows at any cell width
      static const char *const cond_short[7] = {
          "CLR", "CLD", "FOG", "RN", "SNW", "STM", "N-A" };
      const int avail = cell_w - (wico ? 10 : 0) - temp_w - 4 - pad;
      if (avail > 0) {
        const GRect cond_rect = GRect(ix, y_line - l14, avail, 14 + l14);
        const char *cond_txt = s_cond_buf;
        GSize cond_size = graphics_text_layout_get_content_size(
            cond_txt, font, cond_rect, GTextOverflowModeTrailingEllipsis,
            GTextAlignmentLeft);
        if (cond_size.w > avail) {
          cond_txt = cond_short[prv_cond_index(s_cond_buf)];
          cond_size = graphics_text_layout_get_content_size(
              cond_txt, font, cond_rect, GTextOverflowModeTrailingEllipsis,
              GTextAlignmentLeft);
        }
        if (cond_size.w <= avail) {
          graphics_draw_text(ctx, cond_txt, font, cond_rect,
                             GTextOverflowModeTrailingEllipsis,
                             GTextAlignmentLeft, NULL);
          tx = ix + cond_size.w + 4;
        }
      }
      graphics_draw_text(ctx, s_temp_buf, font,
                         GRect(tx, y_line - l14, temp_w, 14 + l14),
                         GTextOverflowModeTrailingEllipsis,
                         GTextAlignmentLeft, NULL);
      break;
    }
    case 1:  // Wind
      {
        const char *wind_txt = (cell_w < 70) ? s_wind_short : s_wind_buf;
        draw_wind_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, wind_txt, font,
                           GRect(cell_x + 10, y_line - l14, cell_w - 10 - pad,
                                 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 2:  // Humidity
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_drop_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, s_hum_buf, font,
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 3:  // UV
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_uv_icon(ctx, GPoint(cell_x, y_line + 3));
        graphics_draw_text(ctx, s_uv_buf, font,
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 4:  // Sunrise
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), false);
        graphics_draw_text(ctx, s_sun_rise, font,
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 5:  // Sunset
      {
        const bool wide = (cell_w >= 40);  // narrow cells: text only
        if (wide) draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), true);
        graphics_draw_text(ctx, s_sun_set, font,
                           GRect(cell_x + (wide ? 10 : 0), y_line - l14,
                                 cell_w - (wide ? 10 : 0) - pad, 14 + l14),
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

  // Collect active metrics (fixed order). Flint (any layout width): the
  // short rows can't fit sunrise/sunset reliably — dropped.
  const bool narrow = (PBL_DISPLAY_WIDTH < 200);
  int act[6];
  int n = 0;
  for (int i = 0; i < 6; i++) {
    if (narrow && (i == 4 || i == 5)) continue;
    if (mask & (1u << i)) act[n++] = i;
  }

  // Pack into rows: full = own row; halves pair up; lone trailing half
  // centered. If the block overflows the content height, a second pass
  // demotes the full-width metrics (weather, wind) to half cells sharing
  // one line — frees a whole row. If it still overflows, the whole panel
  // retries on the 12px metric face, then trailing rows are dropped.
  const int half = w / 2;
  int rows[6][2];            // {metric, cell_x}; metric -1 = none
  int row_w[6];              // cell width for the row's first cell
  int row_w2[6];             // cell width for the row's second cell
  int row_count = 0;
  int line_h = 14;           // 14px lines; retried at 12px if it overflows
  bool demoted = false;
  bool fits = false;
  bool split_ok = true;      // wind pair split measured OK at this face
  for (int fpass = 0; fpass < 2 && !fits; fpass++) {
    for (int pass = 0; pass < 2 && !fits; pass++) {
      row_count = 0;
      split_ok = true;
      demoted = (pass > 0);  // 2nd pass: fulls share half cells, frees a row
      const GFont pf = fonts_get((fpass == 0) ? FONT_SIZE_HEADER
                                              : FONT_SIZE_METRIC);
      for (int i = 0; i < n; ) {
        if ((act[i] <= 1) && !demoted) {      // full-width metric
          rows[row_count][0] = act[i];
          rows[row_count][1] = -1;
          row_w[row_count] = w;
          row_w2[row_count] = w;
          row_count++;
          i++;
        } else {                              // half metric (or demoted full)
          rows[row_count][0] = act[i];
          rows[row_count][1] = -1;
          row_w[row_count] = half;
          if (i + 1 < n && (demoted || act[i + 1] > 1)) {  // pair with next
            rows[row_count][1] = act[i + 1];
            const int a = act[i], b = act[i + 1];
            if (a == 1 || b == 1) {
              // Wind pairs: measured split — the wind cell is sized on its
              // text (icon first, then icon dropped) while the other cell
              // keeps at least its own minimum (temp or half). If that is
              // impossible, the pair falls back to 50/50 on the next pass.
              const int wind_w = (int)graphics_text_layout_get_content_size(
                  s_wind_short, pf, GRect(0, 0, w, 20),
                  GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
              const int other_min =
                  (a == 0 || b == 0)
                      ? (int)graphics_text_layout_get_content_size(
                            s_temp_buf, pf, GRect(0, 0, w, 20),
                            GTextOverflowModeTrailingEllipsis,
                            GTextAlignmentLeft).w
                      : half;
              int wind_cell = 10 + wind_w;  // icon + text
              int other_cell = w - 3 - wind_cell;
              if (other_cell < other_min) {
                wind_cell = wind_w;         // wind icon dropped
                other_cell = w - 3 - wind_cell;
              }
              if (other_cell < other_min) {
                split_ok = false;  // retry on the next font pass
              } else if (a == 1) {
                row_w[row_count] = wind_cell + 3;  // left pad rides along
                row_w2[row_count] = other_cell;
              } else {
                row_w[row_count] = other_cell;
                row_w2[row_count] = wind_cell;
              }
            }
            i += 2;
          } else {
            i++;                              // lone trailing half — centered
          }
          row_count++;
        }
      }
      fits = split_ok && (row_count * line_h + (row_count - 1) <= ch);
    }
    if (!fits) line_h = 12;  // retry packed with the 12px metric face
  }
  if (!fits && row_count > 1) {
    // Last resort: drop trailing rows until it fits
    while (row_count > 1 && row_count * line_h + (row_count - 1) > ch) {
      row_count--;
    }
  }

  // Vertical block: one line per row, 1px gaps, centered in content
  const int gap = 1;
  const int block_h = row_count * line_h + (row_count - 1) * gap;
  int y0 = y + (ch - block_h) / 2;
  if (y0 < y) y0 = y;

  const GFont font = fonts_get((line_h == 12) ? FONT_SIZE_METRIC
                                              : FONT_SIZE_HEADER);
  const int l = (line_h == 12) ? FONT_LEADING_12 : FONT_LEADING_14;
  for (int r = 0; r < row_count; r++) {
    int y_line = y0 + r * (line_h + gap);
    int cell_x = x;
    if (rows[r][0] > 1 && rows[r][1] < 0) {
      cell_x = x + (w - half) / 2;        // lone half — centered
    }
    prv_draw_metric(ctx, rows[r][0], cell_x, row_w[r], y_line, 3, font, l);
    if (rows[r][1] >= 0) {
      prv_draw_metric(ctx, rows[r][1], x + row_w[r], row_w2[r], y_line, 0,
                      font, l);
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
