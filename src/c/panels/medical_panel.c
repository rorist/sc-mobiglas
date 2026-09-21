#include "medical_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_hr_buf[12];
static char s_steps_buf[12];   // "12345" or "---"
static int  s_hr_pct;          // 0..100 mapped from 40..180 bpm
static int  s_steps_pct;       // 0..100 steps_today / avg_daily
static int  s_hr_bpm;          // raw bpm, <=0 = unavailable

static void prv_refresh_health(void) {
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  s_hr_bpm = (int)hr;
  if (hr > 0) {
    snprintf(s_hr_buf, sizeof(s_hr_buf), "%d", (int)hr);
    int pct = ((int)hr - 40) * 100 / (180 - 40);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    s_hr_pct = pct;
  } else {
    snprintf(s_hr_buf, sizeof(s_hr_buf), "---");
    s_hr_pct = 0;
  }

  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  if (steps > 0) {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "%d", (int)steps);
  } else {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "---");
  }

  // Steps goal baseline: user's daily average, fallback 10000
  time_t now = time(NULL);
  time_t start = time_start_of_today();
  HealthValue avg = 0;
  if (health_service_metric_averaged_accessible(HealthMetricStepCount, start, now,
        HealthServiceTimeScopeDaily) == HealthServiceAccessibilityMaskAvailable) {
    avg = health_service_sum_averaged(HealthMetricStepCount, start, now,
                                      HealthServiceTimeScopeDaily);
  }
  int goal = (avg > 0) ? (int)avg : 10000;
  if (goal <= 0) goal = 10000;
  int spct = (steps > 0) ? ((int)steps * 100 / goal) : 0;
  if (spct < 0) spct = 0;
  if (spct > 100) spct = 100;
  s_steps_pct = spct;
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "MEDICAL", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  int cx = content.origin.x;
  int cy = content.origin.y;
  int cw = content.size.w;
  int ch = content.size.h;

  // HR ring color: safe(green) 50..100 bpm, else warn(orange)
  GColor hr_col = (s_hr_bpm >= 50 && s_hr_bpm <= 100) ? COLOR_SAFE : COLOR_WARN;

  graphics_context_set_text_color(ctx, COLOR_TEXT);

  bool compact = (cw < 120);
  if (compact) {
    // Two side-by-side mini ring gauges, values + labels below (SC style)
    int rd = ch - 31;
    if (rd > 28) rd = 28;
    if (rd < 20) rd = 20;
    int col_l = cx + cw / 4 - rd / 2;
    int col_r = cx + 3 * cw / 4 - rd / 2;
    int y0 = cy + (ch - (rd + 31)) / 2;
    if (y0 < cy) y0 = cy;

    draw_ring_gauge(ctx, GRect(col_l, y0, rd, rd),
                    s_hr_pct, 3, COLOR_SECONDARY, hr_col);
    draw_ring_gauge(ctx, GRect(col_r, y0, rd, rd),
                    s_steps_pct, 3, COLOR_SECONDARY, COLOR_PRIMARY);

    int val_y = y0 + rd + 1;
    graphics_draw_text(ctx, s_hr_buf, fonts_get(FONT_SIZE_HEADER),
                       GRect(cx, val_y, cw / 2, 16),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, s_steps_buf, fonts_get(FONT_SIZE_HEADER),
                       GRect(cx + cw / 2, val_y, cw / 2, 16),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);

    int lbl_y = val_y + 15;
    graphics_draw_text(ctx, "BPM", fonts_get(FONT_SIZE_HEADER),
                       GRect(cx, lbl_y, cw / 2, 14),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    graphics_draw_text(ctx, "STEPS", fonts_get(FONT_SIZE_HEADER),
                       GRect(cx + cw / 2, lbl_y, cw / 2, 14),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  } else {
    // Ring box: square, centered horizontally, sized to available height
    int ring_d = ch - 22;             // leave room for values below
    if (ring_d > cw) ring_d = cw;
    if (ring_d < 24) ring_d = 24;
    int ring_x = cx + (cw - ring_d) / 2;
    GRect ring_box = GRect(ring_x, cy, ring_d, ring_d);

    draw_dual_ring(ctx, ring_box, s_hr_pct, hr_col, s_steps_pct, COLOR_PRIMARY);

    // BPM centered in ring, labels+values below
    GRect hr_c = GRect(cx, cy + ring_d / 2 - 12, cw, 20);
    graphics_draw_text(ctx, s_hr_buf, fonts_get(FONT_SIZE_VALUE), hr_c,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    int ty = cy + ring_d + 2;
    GRect hr_lbl = GRect(cx, ty, cw / 2, 16);
    graphics_draw_text(ctx, "BPM", fonts_get(FONT_SIZE_HEADER), hr_lbl,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentLeft, NULL);
    char steps_lbl[20];
    snprintf(steps_lbl, sizeof(steps_lbl), "STEPS %s", s_steps_buf);
    GRect st_lbl = GRect(cx + cw / 2, ty, cw / 2, 16);
    graphics_draw_text(ctx, steps_lbl, fonts_get(FONT_SIZE_HEADER), st_lbl,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentRight, NULL);
  }
}

Layer *medical_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);
  prv_refresh_health();
  return s_layer;
}

void medical_panel_destroy(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void medical_panel_update_bounds(GRect bounds) {
  if (s_layer) {
    layer_set_frame(s_layer, bounds);
    prv_refresh_health();
    layer_mark_dirty(s_layer);
  }
}
