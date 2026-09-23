#include "time_panel.h"
#include "panel.h"
#include "../watchface.h"

static Layer *s_layer;
static char s_time_buf[6];   // "HH:MM\0"
static char s_date_buf[16];  // "TUE 01 JUL\0"
static GBitmap *s_logo_bmp;

// (Re)load the constructor logo bitmap matching watchface_get_logo()
static void prv_load_logo(void) {
  if (s_logo_bmp) {
    gbitmap_destroy(s_logo_bmp);
    s_logo_bmp = NULL;
  }
  uint32_t res;
  switch (watchface_get_logo()) {
    case 1:  res = RESOURCE_ID_IMAGE_LOGO_AEGIS;       break;
    case 2:  res = RESOURCE_ID_IMAGE_LOGO_ANVIL;       break;
    case 3:  res = RESOURCE_ID_IMAGE_LOGO_CRUSADER;    break;
    case 4:  res = RESOURCE_ID_IMAGE_LOGO_RSI;         break;
    case 5:  res = RESOURCE_ID_IMAGE_LOGO_DRAKE;       break;
    case 6:  res = RESOURCE_ID_IMAGE_LOGO_ORIGIN;      break;
    case 7:  res = RESOURCE_ID_IMAGE_LOGO_STARCITIZEN; break;
    case 8:  res = RESOURCE_ID_IMAGE_LOGO_FRONTIER;    break;
    case 9:  res = RESOURCE_ID_IMAGE_LOGO_HEADHUNTERS; break;
    default: return;
  }
  s_logo_bmp = gbitmap_create_with_resource(res);
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Panel chrome + header label
  panel_draw_header_full(ctx, bounds, "NAVCOMP", COLOR_PRIMARY);

  // Dynamic layout: time zone scales with the font (logo present = 50px in
  // 52px zone, no logo = 60px in 56px zone), gap 3px, date 18px.
  GRect content = panel_content_rect(bounds);
#if PBL_DISPLAY_WIDTH < 200
  // Flint (144px wide): logo + time can't fit side by side — logo not
  // rendered, time uses the 48px font at full content width
  const bool show_logo = false;
  const int time_rect_h = 44;  // 48px font zone
#else
  const bool show_logo = (s_logo_bmp != NULL);
  const int time_rect_h = s_logo_bmp ? 52 : 56;
#endif
  int block_h = time_rect_h + 3 + 18;
  int y_offset = content.origin.y + (content.size.h - block_h) / 2;
  if (y_offset < content.origin.y) y_offset = content.origin.y;

  // Dynamic time font: 60px without logo (186px zone fits any time),
  // 50px with logo (116px zone — worst time "04:44" = 114.6px, fits).
  // Rajdhani is variable-width; larger + logo would truncate wide times.
  const int l_time = show_logo ? FONT_LEADING_50 : FONT_LEADING_60;
  const FontSize time_font = show_logo ? FONT_SIZE_TIME : FONT_SIZE_TIME_BIG;
  const int l18 = FONT_LEADING_18;

  // Fixed 64px logo zone at the right edge — time AND date keep the
  // same position whatever logo is active
  GRect logo_bounds = show_logo ? gbitmap_get_bounds(s_logo_bmp) : GRectZero;
  const int LOGO_ZONE_W = 64;
  const int LOGO_GAP = 6;
  int time_w = content.size.w;
  if (show_logo) time_w -= LOGO_ZONE_W + LOGO_GAP;
  GRect time_rect = GRect(content.origin.x, y_offset - l_time,
                          time_w, time_rect_h + l_time);

  graphics_context_set_text_color(ctx, watchface_get_color_time());
  graphics_draw_text(ctx, s_time_buf, fonts_get(time_font), time_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Date: DOW DD MON — same width as time zone (aligned with it)
  GRect date_rect = GRect(content.origin.x,
                           y_offset + time_rect_h + 3 - l18,
                           time_w, 18 + l18);
  graphics_context_set_text_color(ctx, watchface_get_color_value());
  graphics_draw_text(ctx, s_date_buf, fonts_get(FONT_SIZE_VALUE), date_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Constructor logo — right side, full-width 64px, height varies per logo.
  // Vertically + horizontally centered in the fixed right zone.
  if (show_logo) {
    int logo_y = content.origin.y
               + (content.size.h - logo_bounds.size.h) / 2;
    if (logo_y < content.origin.y) logo_y = content.origin.y;
    GRect logo_rect = GRect(
        content.origin.x + content.size.w - LOGO_ZONE_W
            + (LOGO_ZONE_W - logo_bounds.size.w) / 2,
        logo_y,
        logo_bounds.size.w, logo_bounds.size.h);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_logo_bmp, logo_rect);
  }
}

Layer *time_panel_create(GRect bounds) {
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, prv_update_proc);

  prv_load_logo();

  // Initialize with current time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  time_panel_update(t);

  return s_layer;
}

void time_panel_destroy(void) {
  if (s_logo_bmp) {
    gbitmap_destroy(s_logo_bmp);
    s_logo_bmp = NULL;
  }
  if (s_layer) {
    layer_destroy(s_layer);
    s_layer = NULL;
  }
}

void time_panel_update(struct tm *tick_time) {
  if (watchface_get_config() & CONFIG_12H) {
    strftime(s_time_buf, sizeof(s_time_buf), "%I:%M", tick_time);
    if (s_time_buf[0] == '0') {
      memmove(s_time_buf, s_time_buf + 1, sizeof(s_time_buf) - 1);
    }
  } else {
    strftime(s_time_buf, sizeof(s_time_buf), "%H:%M", tick_time);
  }

  strftime(s_date_buf, sizeof(s_date_buf), "%a %d %b", tick_time);

  // Uppercase
  for (char *p = s_date_buf; *p; p++) {
    if (*p >= 'a' && *p <= 'z') *p -= 32;
  }

  if (s_layer) layer_mark_dirty(s_layer);
}

void time_panel_refresh(void) {
  prv_load_logo();
  if (s_layer) layer_mark_dirty(s_layer);
}

