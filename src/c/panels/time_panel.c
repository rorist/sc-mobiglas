#include "time_panel.h"
#include "panel.h"

static Layer *s_layer;
static char s_time_buf[6];   // "HH:MM\0"
static char s_date_buf[16];  // "TUE 01 JUL\0"

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Panel chrome: border + corner accents + header
  panel_draw_header_full(ctx, bounds, "TIME", COLOR_PRIMARY);

  // Content area below header
  GRect content = panel_content_rect(bounds);

  // Time: HH:MM — large centered text
  GRect time_rect = GRect(content.origin.x, content.origin.y - 4,
                          content.size.w, 48);
  graphics_context_set_text_color(ctx, COLOR_TEXT);
  graphics_draw_text(ctx, s_time_buf, fonts_get(FONT_SIZE_TIME), time_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Date: DOW DD MON — smaller centered text below time
  GRect date_rect = GRect(content.origin.x, content.origin.y + 30,
                           content.size.w, 18);
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
  // Format time — 24h for now (config will add 12h later)
  strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", tick_time);

  // Format date: "TUE 01 JUL"
  strftime(s_date_buf, sizeof(s_date_buf), "%a %d %b", tick_time);

  // Uppercase the date string
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') {
      *p -= 32;
    }
  }

  if (s_layer) {
    layer_mark_dirty(s_layer);
  }
}
