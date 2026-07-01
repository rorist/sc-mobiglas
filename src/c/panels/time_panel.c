#include "time_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_time_buf[6];   // "HH:MM\0"
static char s_date_buf[16];  // "TUE 01 JUL\0"

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Panel chrome: border + corner accents (no header label for TIME)
  panel_draw_chrome(ctx, bounds, COLOR_PRIMARY);

  // Content block: 48px time + ~16px date = ~68px total
  // Center this block vertically within the panel
  int content_h = 68;
  int y_offset = (bounds.size.h - content_h) / 2;
  if (y_offset < 4) y_offset = 4;

  // Time: HH:MM — large centered text
  GRect time_rect = GRect(bounds.origin.x + 4,
                           bounds.origin.y + y_offset,
                           bounds.size.w - 8, 50);
  graphics_context_set_text_color(ctx, COLOR_TEXT);
  graphics_draw_text(ctx, s_time_buf, fonts_get(FONT_SIZE_TIME), time_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Date: DOW DD MON — smaller centered text below time
  GRect date_rect = GRect(bounds.origin.x + 4,
                           bounds.origin.y + y_offset + 50,
                           bounds.size.w - 8, 18);
  graphics_draw_text(ctx, s_date_buf, fonts_get(FONT_SIZE_HEADER), date_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

Layer *time_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);

  // Initialize with current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  time_panel_update(t);

  return s_layer;
}

void time_panel_destroy(void) {
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void time_panel_update(struct tm *tick_time) {
  strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", tick_time);

  strftime(s_date_buf, sizeof(s_date_buf), "%a %d %b", tick_time);

  // Uppercase
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 32;
  }

  if (s_layer) layer_mark_dirty(s_layer);
}

void time_panel_update_bounds(GRect bounds) {
  if (s_layer) {
    layer_set_frame(s_layer, bounds);
    layer_mark_dirty(s_layer);
  }
}
