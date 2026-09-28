#include "medical_panel.h"
#include "panel.h"
#include "../watchface.h"

#define MED_SLOT_COUNT 4
#define MED_SLEEP_GOAL_S (8 * 3600)   // 8h sleep goal
#define MED_KCAL_GOAL_FALLBACK 500

// One metric slot: label + formatted value + gauge percent + colors.
// Full mode renders all 4 slots as one row of rings; compact mode renders
// slots 0-1 only. Future configurable metrics (#13) plug in here.
typedef struct {
  const char *label;
  char value[12];
  int pct;
  GColor fill;
  GColor value_col;
} MedSlot;

static Layer *s_layer;
static MedSlot s_slots[MED_SLOT_COUNT];

static void prv_fill_hr(void) {
  MedSlot *s = &s_slots[0];
  s->label = "BPM";
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  int bpm = (int)hr;
  bool normal = (bpm >= 50 && bpm <= 100);
  if (bpm > 0) {
    snprintf(s->value, sizeof(s->value), "%d", bpm);
    int pct = (bpm - 40) * 100 / (180 - 40);
    s->pct = (pct < 0) ? 0 : (pct > 100 ? 100 : pct);
  } else {
    snprintf(s->value, sizeof(s->value), "---");
    s->pct = 0;
  }
  s->fill = normal ? COLOR_SAFE : watchface_get_color_warn();
  s->value_col = normal ? watchface_get_color_value()
                        : watchface_get_color_warn();
}

static void prv_fill_steps(void) {
  MedSlot *s = &s_slots[1];
  s->label = "STEPS";
  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  if (steps > 0) {
    snprintf(s->value, sizeof(s->value), "%d", (int)steps);
  } else {
    snprintf(s->value, sizeof(s->value), "---");
  }

  // Goal: user's daily average, fallback 10000
  time_t now = time(NULL);
  time_t start = time_start_of_today();
  HealthValue goal = 0;
  if (health_service_metric_averaged_accessible(HealthMetricStepCount, start,
        now, HealthServiceTimeScopeDaily)
        == HealthServiceAccessibilityMaskAvailable) {
    goal = health_service_sum_averaged(HealthMetricStepCount, start, now,
                                       HealthServiceTimeScopeDaily);
  }
  if (goal <= 0) goal = 10000;
  int pct = (steps > 0) ? ((int)steps * 100 / (int)goal) : 0;
  s->pct = (pct < 0) ? 0 : (pct > 100 ? 100 : pct);
  s->fill = watchface_get_color_label();
  s->value_col = watchface_get_color_value();
}

static void prv_fill_sleep(void) {
  MedSlot *s = &s_slots[2];
  s->label = "SLEEP";
  HealthValue secs = health_service_sum_today(HealthMetricSleepSeconds);
  if (secs > 0) {
    int h = (int)secs / 3600;
    int m = ((int)secs % 3600) / 60;
    snprintf(s->value, sizeof(s->value), "%dh%02d", h, m);
    int pct = (int)secs * 100 / MED_SLEEP_GOAL_S;
    s->pct = (pct > 100) ? 100 : pct;
  } else {
    snprintf(s->value, sizeof(s->value), "---");
    s->pct = 0;
  }
  s->fill = watchface_get_color_label();
  s->value_col = watchface_get_color_value();
}

static void prv_fill_kcal(void) {
  MedSlot *s = &s_slots[3];
  s->label = "KCAL";
  HealthValue kcal = health_service_sum_today(HealthMetricActiveKCalories);
  if (kcal > 0) {
    snprintf(s->value, sizeof(s->value), "%d", (int)kcal);
  } else {
    snprintf(s->value, sizeof(s->value), "---");
  }

  // Goal: user's daily average, fallback 500
  time_t now = time(NULL);
  time_t start = time_start_of_today();
  HealthValue goal = 0;
  if (health_service_metric_averaged_accessible(HealthMetricActiveKCalories,
        start, now, HealthServiceTimeScopeDaily)
        == HealthServiceAccessibilityMaskAvailable) {
    goal = health_service_sum_averaged(HealthMetricActiveKCalories, start, now,
                                       HealthServiceTimeScopeDaily);
  }
  if (goal <= 0) goal = MED_KCAL_GOAL_FALLBACK;
  int pct = (kcal > 0) ? ((int)kcal * 100 / (int)goal) : 0;
  s->pct = (pct < 0) ? 0 : (pct > 100 ? 100 : pct);
  s->fill = watchface_get_color_label();
  s->value_col = watchface_get_color_value();
}

static void prv_refresh_health(void) {
  prv_fill_hr();
  prv_fill_steps();
  prv_fill_sleep();
  prv_fill_kcal();
}

// Draw one ring cell: ring gauge with value centered inside, label below.
static void prv_draw_ring_cell(GContext *ctx, int x, int y, int rd,
                               int cell_w, const MedSlot *slot) {
  draw_ring_gauge(ctx, GRect(x, y, rd, rd), slot->pct, 3,
                  COLOR_GAUGE_BG, slot->fill);

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

  if (cw >= 120) {
    // Full mode: one row of 4 ring gauges (HR, STEPS, SLEEP, KCAL)
    const int cell_w = cw / MED_SLOT_COUNT;
    int rd = ch - 17;
    int rd_max = cell_w - 6;
    if (rd > rd_max) rd = rd_max;
    if (rd < 16) rd = 16;

    int y0 = cy + (ch - (rd + 16)) / 2;
    if (y0 < cy) y0 = cy;

    for (int i = 0; i < MED_SLOT_COUNT; i++) {
      int colx = cx + i * cell_w + (cell_w - rd) / 2;
      prv_draw_ring_cell(ctx, colx, y0, rd, cell_w, &s_slots[i]);
    }
  } else {
    // Compact mode: two side-by-side ring gauges (HR, STEPS)
    int rd = ch - 17;                   // ring + 2px gap + 14px label
    int rd_max = cw / 2 - 8;            // fit within own column
    if (rd > rd_max) rd = rd_max;
    if (rd < 20) rd = 20;
    int col_l = cx + cw / 4 - rd / 2;
    int col_r = cx + 3 * cw / 4 - rd / 2;
    int y0 = cy + (ch - (rd + 16)) / 2; // block (ring+label) centered below header
    if (y0 < cy) y0 = cy;

    prv_draw_ring_cell(ctx, col_l, y0, rd, cw / 2, &s_slots[0]);
    prv_draw_ring_cell(ctx, col_r, y0, rd, cw / 2, &s_slots[1]);
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
