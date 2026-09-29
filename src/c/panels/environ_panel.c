#include "environ_panel.h"
#include "panel.h"
#include "../ui/draw_utils.h"
#include <string.h>

static Layer *s_layer;
static char s_temp_buf[12];     // "-12°C" or "--°C"
static char s_cond_buf[12];     // "CLOUDY" or "---"
static char s_cond_raw[12];     // raw condition, survives rebuilds (like s_last_temp_c)
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
// Flint (1-bit): icons degrade to a 2x2 dot — a glyph-free primitive that
// keeps narrow cells compact (user decision 2026-09-29)
static void prv_draw_dot(GContext *ctx, GPoint p) {
  graphics_context_set_fill_color(ctx, watchface_get_color_label());
  graphics_fill_rect(ctx, GRect(p.x, p.y, 2, 2), 0, GCornerNone);
}

static void prv_draw_metric(GContext *ctx, int metric, int cell_x, int cell_w,
                            int y_line, int pad, GFont font, int l) {
  const int l14 = l;
  const bool dots = (PBL_DISPLAY_WIDTH < 200);  // flint: icons -> dots
  const int iw = dots ? 4 : 10;      // icon+gap width (dot 2+2 vs icon 8+2)
  const int wide_min = dots ? 34 : 40;
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
      const bool wico = (cell_w >= iw + temp_w);
      const int ix = cell_x + (wico ? iw : 0);
      if (wico) {
        if (dots) {
          prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
        } else {
          draw_weather_icon(ctx, GPoint(cell_x, y_line + 2),
                            prv_cond_index(s_cond_buf));
        }
      }
      int tx = ix;  // default: right after the icon (or cell start)
      // Condition: full text if it fits before the temp, short form
      // (CLR/CLD/FOG/RN/SNW/STM) otherwise, icon only as a last resort —
      // the condition now shows at any cell width
      static const char *const cond_short[7] = {
          "CLR", "CLD", "FOG", "RN", "SNW", "STM", "N-A" };
      const int avail = cell_w - (wico ? iw : 0) - temp_w - 4 - pad;
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
        // Full text (with direction) when the cell can hold it, short form
        // otherwise; the icon rides along only when text + icon fit —
        // ultra-narrow measured splits fall back to text only.
        const int wfull = (int)graphics_text_layout_get_content_size(
            s_wind_buf, font, GRect(0, 0, 200, 20),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
        const int wshort = (int)graphics_text_layout_get_content_size(
            s_wind_short, font, GRect(0, 0, 200, 20),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
        const bool full = (cell_w >= iw + wfull);
        const bool wico = full || (cell_w >= iw + wshort);
        const char *wind_txt = full ? s_wind_buf : s_wind_short;
        if (wico) {
          if (dots) {
            prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
          } else {
            draw_wind_icon(ctx, GPoint(cell_x, y_line + 3));
          }
        }
        graphics_draw_text(
            ctx, wind_txt, font,
            GRect(cell_x + (wico ? iw : 0), y_line - l14,
                  cell_w - (wico ? iw : 0) - pad, 14 + l14),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      }
      break;
    case 2:  // Humidity
      {
        const bool wide = (cell_w >= wide_min);  // narrow cells: text only
        if (wide) {
          if (dots) {
            prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
          } else {
            draw_drop_icon(ctx, GPoint(cell_x, y_line + 3));
          }
        }
        graphics_draw_text(ctx, s_hum_buf, font,
                           GRect(cell_x + (wide ? iw : 0), y_line - l14,
                                 cell_w - (wide ? iw : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 3:  // UV
      {
        const bool wide = (cell_w >= wide_min);  // narrow cells: text only
        if (wide) {
          if (dots) {
            prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
          } else {
            draw_uv_icon(ctx, GPoint(cell_x, y_line + 3));
          }
        }
        graphics_draw_text(ctx, s_uv_buf, font,
                           GRect(cell_x + (wide ? iw : 0), y_line - l14,
                                 cell_w - (wide ? iw : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 4:  // Sunrise
      {
        const bool wide = (cell_w >= wide_min);  // narrow cells: text only
        if (wide) {
          if (dots) {
            prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
          } else {
            draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), false);
          }
        }
        graphics_draw_text(ctx, s_sun_rise, font,
                           GRect(cell_x + (wide ? iw : 0), y_line - l14,
                                 cell_w - (wide ? iw : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
    case 5:  // Sunset
      {
        const bool wide = (cell_w >= wide_min);  // narrow cells: text only
        if (wide) {
          if (dots) {
            prv_draw_dot(ctx, GPoint(cell_x, y_line + 6));
          } else {
            draw_sun_icon(ctx, GPoint(cell_x, y_line + 3), true);
          }
        }
        graphics_draw_text(ctx, s_sun_set, font,
                           GRect(cell_x + (wide ? iw : 0), y_line - l14,
                                 cell_w - (wide ? iw : 0) - pad, 14 + l14),
                           GTextOverflowModeTrailingEllipsis,
                           GTextAlignmentLeft, NULL);
      }
      break;
  }
}

// Measured content width of one metric (icon + text) at the given face.
// wide (solo flow layout): wind keeps its direction, weather reserves room
// for the full condition text; narrow: degradation forms apply.
static int prv_item_need_w(int metric, GFont pf, bool wide) {
  const int iw = (PBL_DISPLAY_WIDTH < 200) ? 4 : 10;  // icon+gap width
  const int min_wide = (PBL_DISPLAY_WIDTH < 200) ? 34 : 40;
  if (metric == 0) {
    const int temp_w = (int)graphics_text_layout_get_content_size(
        s_temp_buf, pf, GRect(0, 0, 200, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
    if (!wide) return iw + temp_w + 7;  // +7: pair pad + measure underestimate
    const int cond_w = (int)graphics_text_layout_get_content_size(
        s_cond_buf, pf, GRect(0, 0, 200, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
    return iw + cond_w + 4 + temp_w + 2;
  }
  const char *txt = (metric == 1) ? (wide ? s_wind_buf : s_wind_short)
                  : (metric == 2) ? s_hum_buf
                  : (metric == 3) ? s_uv_buf
                  : (metric == 4) ? s_sun_rise
                                  : s_sun_set;
  int wd = iw + (int)graphics_text_layout_get_content_size(
                   txt, pf, GRect(0, 0, 200, 20),
                   GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft)
                   .w;
  if (wide && wd < min_wide) {
    wd = min_wide;  // keep the icon readable in flow layout
  }
  wd += wide ? 5 : 7;  // wide: ellipsis guard; narrow: pair pad + underestimate
  return wd;
}

// Full-width solo layout: left-aligned flow — items are laid out in reading
// order and wrap to the next line when the next one doesn't fit. Ladder:
// 14px face, 12px face, then sunrise/sunset dropped as a last resort.
static void prv_draw_flow(GContext *ctx, int x, int y, int w, int ch,
                          const int *act, int n) {
  for (int fpass = 0; fpass < 2; fpass++) {
    const GFont pf =
        fonts_get((fpass == 0) ? FONT_SIZE_HEADER : FONT_SIZE_METRIC);
    const int line_h = (fpass == 0) ? 14 : 12;
    const int l = (fpass == 0) ? FONT_LEADING_14 : FONT_LEADING_12;
    for (int dpass = 0; dpass < 2; dpass++) {
      int idx[6], ix[6], iw[6], irow[6];
      int m = 0, rows = 1, cx = 0;
      for (int i = 0; i < n; i++) {
        if (dpass == 1 && (act[i] == 4 || act[i] == 5)) continue;
        int wd = prv_item_need_w(act[i], pf, true);
        if (wd > w) wd = w;  // oversized single item takes its own line
        // Layout wraps on the strict width: the +5 slack exists only for
        // drawing (ellipsis guard), it must not trigger early wraps
        const int step = wd - 5;
        if (cx > 0 && cx + step > w) {  // wrap to the next line
          rows++;
          cx = 0;
        }
        idx[m] = act[i];
        ix[m] = cx;
        iw[m] = wd;
        irow[m] = rows - 1;
        m++;
        cx += step + 8;
      }
      const int block_h = rows * line_h + (rows - 1);
      if (block_h <= ch) {
        const int y0 = y + (ch - block_h) / 2;
        for (int k = 0; k < m; k++) {
          prv_draw_metric(ctx, idx[k], x + ix[k], iw[k],
                          y0 + irow[k] * (line_h + 1), 0, pf, l);
        }
        return;
      }
    }
  }
}

// Metrics: 0 WEATHER (full), 1 WIND (full), 2 HUM (half), 3 UV (half),
// 4 SUNRISE (half), 5 SUNSET (half). Wide layout (panel solo, w >= 120):
// left-aligned flow. Narrow layout (side-by-side panels): full metrics take
// their own line when the height budget allows, halves pair up two per
// line, lone halves sit left-aligned.
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

  // Full-width solo layout (panel alone on its row, w >= 120): items flow
  // left-aligned in reading order and wrap like words on a line.
  if (w >= 120) {
    prv_draw_flow(ctx, x, y, w, ch, act, n);
    return;
  }

  // Narrow layout (side-by-side panels): full = own row when the height
  // budget allows; halves pair up two per line; lone trailing half is
  // left-aligned. If the block overflows the content height, a second pass
  // demotes the full-width metrics (weather, wind) to half cells sharing
  // one line — frees a whole row. If it still overflows, the whole panel
  // retries on the 12px metric face, then trailing rows are dropped.
  const int half = w / 2;
  int rows[6][2];            // {metric, cell_x}; metric -1 = none
  int row_w[6];              // cell width for the row's first cell
  int row_w2[6];             // cell width for the row's second cell
  int row_count = 0;
  int line_h = 14;           // 14px lines; retried at 12px if it overflows
  bool fits = false;
  bool split_ok = true;      // wind pair split measured OK at this face
  for (int fpass = 0; fpass < 2 && !fits; fpass++) {
    for (int pass = 0; pass < 3 && !fits; pass++) {
      row_count = 0;
      split_ok = true;
      // pass k = number of full-width metrics that keep their own row,
      // descending. WEATHER is kept first (the condition matters more than
      // the wind direction); once every full is demoted the remaining
      // passes only retry on the smaller face.
      int nf = 0;  // active full-width metrics (WEATHER, WIND)
      for (int i = 0; i < n; i++) {
        if (act[i] <= 1) nf++;
      }
      const int keep = (pass < nf) ? nf - pass : 0;
      const GFont pf = fonts_get((fpass == 0) ? FONT_SIZE_HEADER
                                              : FONT_SIZE_METRIC);
      for (int i = 0; i < n; ) {
        const bool dem_m = (act[i] <= 1) && (keep < act[i] + 1);
        if ((act[i] <= 1) && !dem_m) {        // full-width metric
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
          if (i + 1 < n && keep < act[i + 1] + 1) {  // pair with next
            rows[row_count][1] = act[i + 1];
            const int a = act[i], b = act[i + 1];
            if (a == 1 || b == 1) {
              // Wind pairs: measured split with an icon-first ladder —
              // 1. strict split where BOTH cells keep their icon,
              // 2. otherwise a retry on the smaller font face,
              // 3. on the last face the wind icon is dropped rather than
              //    failing the pack outright.
              const int wind_w = (int)graphics_text_layout_get_content_size(
                  s_wind_short, pf, GRect(0, 0, w, 20),
                  GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft).w;
              const bool has_weather = (a == 0 || b == 0);
              const int temp_w = has_weather
                  ? (int)graphics_text_layout_get_content_size(
                        s_temp_buf, pf, GRect(0, 0, w, 20),
                        GTextOverflowModeTrailingEllipsis,
                        GTextAlignmentLeft).w
                  : 0;
              // Icon-strict minimum for the other cell: icon + temp when the
              // pair holds WEATHER (the condition degrades on its own), else
              // icon + short text for half metrics.
              const int other_ico_min = has_weather ? 10 + temp_w : 40;
              int wind_cell = 10 + wind_w + 3;  // icon + text + small guard
              int other_cell = w - 3 - wind_cell;
              if (other_cell >= other_ico_min) {
                if (a == 1) {
                  row_w[row_count] = wind_cell + 3;  // left pad rides along
                  row_w2[row_count] = other_cell;
                } else {
                  row_w[row_count] = other_cell;
                  row_w2[row_count] = wind_cell;
                }
              } else if (fpass == 1) {
                // Last face: degrade — wind icon dropped; the other cell
                // keeps at least its own minimum (temp or half need).
                const int other_min = has_weather
                    ? temp_w + 5
                    : prv_item_need_w((a == 1) ? b : a, pf, false) - 4;
                wind_cell = wind_w;  // wind icon dropped
                other_cell = w - 3 - wind_cell;
                if (other_cell < other_min) {
                  split_ok = false;  // retry on the next font pass
                } else if (a == 1) {
                  row_w[row_count] = wind_cell + 3;  // left pad rides along
                  row_w2[row_count] = other_cell;
                } else {
                  row_w[row_count] = other_cell;
                  row_w2[row_count] = wind_cell;
                }
              } else {
                split_ok = false;  // retry on the smaller font face
              }
            } else {
              // Other pairs: measured split — the first cell takes its
              // needed width, the second gets the remainder; 50/50 only
              // when neither order leaves both texts readable.
              int c1 = prv_item_need_w(a, pf, false);
              int c2 = w - c1;
              const int need2 = prv_item_need_w(b, pf, false);
              if (c2 < need2) {
                c2 = need2;
                c1 = w - need2;
              }
              if (c1 < 10) {
                c1 = half;
                c2 = w - half;
              }
              row_w[row_count] = c1;
              row_w2[row_count] = c2;
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
    prv_draw_metric(ctx, rows[r][0], x, row_w[r], y_line, 3, font, l);
    if (rows[r][1] >= 0) {
      prv_draw_metric(ctx, rows[r][1], x + row_w[r], row_w2[r], y_line, 0,
                      font, l);
    }
  }
}

// Rebuild s_cond_buf from the raw condition ("---" until first receipt) —
// called on create/rebuild so the weather condition survives config changes
static void prv_format_cond(void) {
  if (s_cond_raw[0] == '\0') {
    snprintf(s_cond_buf, sizeof(s_cond_buf), "---");
  } else {
    snprintf(s_cond_buf, sizeof(s_cond_buf), "%s", s_cond_raw);
  }
}

Layer *environ_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);

  // Keep s_last_temp_c across rebuilds so config changes re-display last data
  prv_format_temp();
  prv_format_cond();
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
    snprintf(s_cond_raw, sizeof(s_cond_raw), "%s", condition);
    prv_format_cond();
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
