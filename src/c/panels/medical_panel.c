#include "medical_panel.h"
#include "panel.h"
#include "../watchface.h"

#define MED_METRIC_COUNT 8
#define MED_SLEEP_GOAL_S (8 * 3600)        // 8h sleep goal
#define MED_DEEP_GOAL_S (2 * 3600)         // 2h deep sleep goal
#define MED_DIST_GOAL_FALLBACK 5000        // meters
#define MED_ACTIVE_GOAL_FALLBACK 3600      // seconds (1h)
#define MED_RKCAL_GOAL_FALLBACK 1500
#define MED_KCAL_GOAL_FALLBACK 500

// One metric slot: label + formatted value + gauge percent + colors.
// Full mode renders the first 4 active metrics as one row of rings; compact
// mode renders the first 2. Which metrics are active comes from the user's
// metrics mask (watchface_get_med_metrics), in this fixed order.
typedef struct {
  const char *label;
  const char *label_short;  // 2-letter form drawn inside narrow rings
  char value[12];
  int pct;
  GColor fill;
  GColor value_col;
} MedSlot;

static void prv_fill_metric(int idx);

static Layer *s_layer;
static MedSlot s_slots[MED_METRIC_COUNT];

// Debug health overrides (CLI/emulator channel): the emulator provides no
// health data, so MEDICAL would only ever show "---". val >= 0 replaces the
// health API read for that metric (native units: bpm, steps, seconds,
// meters, kcal), val < 0 clears the override. Never persisted, seeded
// once per app run — overrides SURVIVE panel rebuilds (same contract as
// the environ statics); a fresh install resets them naturally.
static int32_t s_debug_val[MED_METRIC_COUNT];
static bool s_debug_seeded = false;

void medical_panel_set_debug(int idx, int32_t val) {
  if (idx < 0 || idx >= MED_METRIC_COUNT) return;
  s_debug_val[idx] = val;
  if (s_layer) layer_mark_dirty(s_layer);
}

static void prv_clamp_pct(MedSlot *s, int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  s->pct = pct;
}

// Averaged daily goal for a metric, or fallback if unavailable.
static HealthValue prv_goal(HealthMetric metric, HealthValue fallback) {
  time_t now = time(NULL);
  time_t start = time_start_of_today();
  if (health_service_metric_averaged_accessible(metric, start, now,
        HealthServiceTimeScopeDaily)
        != HealthServiceAccessibilityMaskAvailable) {
    return fallback;
  }
  HealthValue goal = health_service_sum_averaged(metric, start, now,
                                                 HealthServiceTimeScopeDaily);
  return (goal > 0) ? goal : fallback;
}

static void prv_fill_hr(int idx) {
  MedSlot *s = &s_slots[idx];
  s->label = "BPM";
  s->label_short = "BP";
  HealthValue hr = (s_debug_val[idx] >= 0)
      ? (HealthValue)s_debug_val[idx]
      : health_service_peek_current_value(HealthMetricHeartRateBPM);
  int bpm = (int)hr;
  bool normal = (bpm >= 50 && bpm <= 100);
  if (bpm > 0) {
    snprintf(s->value, sizeof(s->value), "%d", bpm);
    prv_clamp_pct(s, (bpm - 40) * 100 / (180 - 40));
  } else {
    snprintf(s->value, sizeof(s->value), "---");
    s->pct = 0;
  }
  s->fill = normal ? COLOR_SAFE : watchface_get_color_warn();
  s->value_col = normal ? watchface_get_color_value()
                        : watchface_get_color_warn();
}

static void prv_fill_sum_int(int idx, const char *label,
                             const char *label_short, HealthMetric metric,
                             HealthValue goal_fallback, bool km) {
  MedSlot *s = &s_slots[idx];
  s->label = label;
  s->label_short = label_short;
  HealthValue v = (s_debug_val[idx] >= 0)
      ? (HealthValue)s_debug_val[idx]
      : health_service_sum_today(metric);
  if (v > 0) {
    if (km) {
      snprintf(s->value, sizeof(s->value), "%d.%dkm",
               (int)v / 1000, ((int)v % 1000) / 100);
    } else {
      snprintf(s->value, sizeof(s->value), "%d", (int)v);
    }
    prv_clamp_pct(s, (int)v * 100 / (int)prv_goal(metric, goal_fallback));
  } else {
    snprintf(s->value, sizeof(s->value), "---");
    s->pct = 0;
  }
  s->fill = watchface_get_color_label();
  s->value_col = watchface_get_color_value();
}

static void prv_fill_duration(int idx, const char *label,
                              const char *label_short, HealthMetric metric,
                              int goal_s) {
  MedSlot *s = &s_slots[idx];
  s->label = label;
  s->label_short = label_short;
  HealthValue secs = (s_debug_val[idx] >= 0)
      ? (HealthValue)s_debug_val[idx]
      : health_service_sum_today(metric);
  if (secs > 0) {
    int h = (int)secs / 3600;
    int m = ((int)secs % 3600) / 60;
    snprintf(s->value, sizeof(s->value), "%dh%02d", h, m);
    prv_clamp_pct(s, (int)secs * 100 / goal_s);
  } else {
    snprintf(s->value, sizeof(s->value), "---");
    s->pct = 0;
  }
  s->fill = watchface_get_color_label();
  s->value_col = watchface_get_color_value();
}

static void prv_fill_metric(int idx) {
  switch (idx) {
    case 0:
      prv_fill_hr(idx);
      break;
    case 1:  // Steps — goal: daily average, fallback 10000
      prv_fill_sum_int(idx, "STEPS", "ST", HealthMetricStepCount, 10000,
                       false);
      break;
    case 2:  // Sleep
      prv_fill_duration(idx, "SLEEP", "SL", HealthMetricSleepSeconds,
                        MED_SLEEP_GOAL_S);
      break;
    case 3:  // Active kcal — goal: daily average, fallback 500
      prv_fill_sum_int(idx, "KCAL", "KC", HealthMetricActiveKCalories,
                       MED_KCAL_GOAL_FALLBACK, false);
      break;
    case 4:  // Distance — value in km, goal: daily average, fallback 5000 m
      prv_fill_sum_int(idx, "KM", "KM", HealthMetricWalkedDistanceMeters,
                       MED_DIST_GOAL_FALLBACK, true);
      break;
    case 5:  // Active time — goal fallback 1h
      prv_fill_duration(idx, "ACT", "AC", HealthMetricActiveSeconds,
                        MED_ACTIVE_GOAL_FALLBACK);
      break;
    case 6:  // Resting kcal
      prv_fill_sum_int(idx, "RKCAL", "RK", HealthMetricRestingKCalories,
                       MED_RKCAL_GOAL_FALLBACK, false);
      break;
    case 7:  // Deep sleep
      prv_fill_duration(idx, "DEEP", "DP", HealthMetricSleepRestfulSeconds,
                        MED_DEEP_GOAL_S);
      break;
  }
}

// Collect active metric indices (fixed order, filtered by the user's mask).
static int prv_active_slots(uint32_t mask, int max_count, int *out_idx) {
  int n = 0;
  for (int i = 0; i < MED_METRIC_COUNT && n < max_count; i++) {
    if (mask & (1u << i)) out_idx[n++] = i;
  }
  return n;
}

static void prv_refresh_health(void) {
  // Fill only the slots that can be drawn (max 4 rings in full mode,
  // 2 in compact) — skips up to 4 health-service reads per minute.
  int idx[MED_METRIC_COUNT];
  const int n = prv_active_slots(watchface_get_med_metrics(), 4, idx);
  for (int i = 0; i < n; i++) {
    prv_fill_metric(idx[i]);
  }
}

// Draw one ring cell: ring gauge with value centered inside, label below.
// Narrow rings (< 28px, flint): empty gauge + value below, no label.
static void prv_draw_ring_cell(GContext *ctx, int x, int y, int rd,
                               int cell_w, const MedSlot *slot) {
  // 1-bit displays: gauge track #0055aa binarizes to black = invisible
  draw_ring_gauge(ctx, GRect(x, y, rd, rd), slot->pct, 3,
                  PBL_IF_BW_ELSE(GColorWhite, COLOR_GAUGE_BG), slot->fill);

  if (rd < 28) {
    // Narrow ring (flint): 2-letter label inside the ring + value below,
    // both on the 12px metric face — unlabeled rings were unreadable
    const int l12 = FONT_LEADING_12;
    graphics_context_set_text_color(ctx, watchface_get_color_label());
    graphics_draw_text(ctx, slot->label_short, fonts_get(FONT_SIZE_METRIC),
                       GRect(x, y + (rd - 12) / 2 - l12, rd, 12 + l12),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, slot->value_col);
    graphics_draw_text(ctx, slot->value, fonts_get(FONT_SIZE_METRIC),
                       GRect(x + (rd - cell_w) / 2, y + rd + 2 - l12,
                             cell_w, 12 + l12),
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
    return;
  }

  graphics_context_set_text_color(ctx, slot->value_col);
  graphics_draw_text(ctx, slot->value, fonts_get(FONT_SIZE_HEADER),
                     GRect(x, y + (rd - 14) / 2 - FONT_LEADING_14,
                           rd, 14 + FONT_LEADING_14),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  graphics_context_set_text_color(ctx, watchface_get_color_label());
  graphics_draw_text(ctx, slot->label, fonts_get(FONT_SIZE_HEADER),
                     GRect(x + (rd - cell_w) / 2, y + rd + 2 - FONT_LEADING_14,
                           cell_w, 14 + FONT_LEADING_14),
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}


static void prv_update_proc(Layer *layer, GContext *ctx) {
  // Re-read health at draw time (cheap cached reads, no sensor wakeups):
  // the panel is marked dirty every minute by watchface_tick, so values
  // stay fresh without any extra timer or health event subscription.
  prv_refresh_health();

  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "MEDICAL", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  int cx = content.origin.x;
  int cy = content.origin.y;
  int cw = content.size.w;
  int ch = content.size.h;
  uint32_t mask = watchface_get_med_metrics();

  int idx[MED_METRIC_COUNT];
  if (cw >= 120) {
    // Full mode: one row of up to 4 active ring gauges. Fewer, larger
    // rings when cells get too narrow to stay readable (values/labels
    // clip at ~34px cells on gabbro med-4; emery med-4 keeps its 4 rings)
    int n = prv_active_slots(mask, 4, idx);
    while (n > 2 && cw / n < 44) n--;
    if (n == 0) return;
    const int cell_w = cw / n;
    int rd = ch - 17;
    int rd_max = cell_w - 6;
    if (rd > rd_max) rd = rd_max;
    if (rd < 16) rd = 16;

    int y0 = cy + (ch - (rd + 16)) / 2;
    if (y0 < cy) y0 = cy;

    for (int i = 0; i < n; i++) {
      int colx = cx + i * cell_w + (cell_w - rd) / 2;
      prv_draw_ring_cell(ctx, colx, y0, rd, cell_w, &s_slots[idx[i]]);
    }
  } else {
    // Compact mode: two side-by-side ring gauges (first 2 active)
    int n = prv_active_slots(mask, 2, idx);
    if (n == 0) return;
    int rd = ch - 17;                   // ring + 2px gap + 14px label
    int rd_max = cw / 2 - 8;            // fit within own column
    if (rd > rd_max) rd = rd_max;
    if (rd < 20) rd = 20;
    int col_l = cx + cw / 4 - rd / 2;
    int col_r = cx + 3 * cw / 4 - rd / 2;
    int y0 = cy + (ch - (rd + 16)) / 2; // block centered below header
    if (y0 < cy) y0 = cy;

    prv_draw_ring_cell(ctx, col_l, y0, rd, cw / 2, &s_slots[idx[0]]);
    if (n > 1) {
      prv_draw_ring_cell(ctx, col_r, y0, rd, cw / 2, &s_slots[idx[1]]);
    }
  }
}

Layer *medical_panel_create(GRect bounds) {
  // Seed once: panel rebuilds (config changes) must NOT clear live debug
  // values — same survive-rebuilds contract as the environ statics.
  if (!s_debug_seeded) {
    for (int i = 0; i < MED_METRIC_COUNT; i++) {
      s_debug_val[i] = -1;  // no override
    }
    s_debug_seeded = true;
  }
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
