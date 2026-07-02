#include "medical_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_hr_buf[12];
static char s_steps_buf[12];   // "12345" or "---"

static void prv_refresh_health(void) {
  HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);
  if (hr > 0) {
    snprintf(s_hr_buf, sizeof(s_hr_buf), "%d", (int)hr);
  } else {
    snprintf(s_hr_buf, sizeof(s_hr_buf), "---");
  }

  HealthValue steps = health_service_sum_today(HealthMetricStepCount);
  if (steps > 0) {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "%d", (int)steps);
  } else {
    snprintf(s_steps_buf, sizeof(s_steps_buf), "---");
  }
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  panel_draw_header_full(ctx, bounds, "MEDICAL", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  graphics_context_set_text_color(ctx, COLOR_TEXT);

  // Stacked: label small on top, value larger below
  // Heart rate label
  GRect hr_lbl = GRect(content.origin.x, content.origin.y,
                        content.size.w, 14);
  graphics_draw_text(ctx, "HEART RATE", fonts_get(FONT_SIZE_HEADER), hr_lbl,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  // Heart rate value
  GRect hr_val = GRect(content.origin.x, content.origin.y + 12,
                        content.size.w, 18);
  graphics_draw_text(ctx, s_hr_buf, fonts_get(FONT_SIZE_VALUE), hr_val,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);

  // Steps label
  GRect st_lbl = GRect(content.origin.x, content.origin.y + 30,
                        content.size.w, 14);
  graphics_draw_text(ctx, "STEPS", fonts_get(FONT_SIZE_HEADER), st_lbl,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
  // Steps value
  GRect st_val = GRect(content.origin.x, content.origin.y + 43,
                        content.size.w, 18);
  graphics_draw_text(ctx, s_steps_buf, fonts_get(FONT_SIZE_VALUE), st_val,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentLeft, NULL);
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
