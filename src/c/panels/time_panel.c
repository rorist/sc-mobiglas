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
  // Index 0 = logo 1 (AEGIS) .. index 8 = logo 9 (HEADHUNTERS)
  static const uint32_t logo_res[9] = {
    RESOURCE_ID_IMAGE_LOGO_AEGIS,
    RESOURCE_ID_IMAGE_LOGO_ANVIL,
    RESOURCE_ID_IMAGE_LOGO_CRUSADER,
    RESOURCE_ID_IMAGE_LOGO_RSI,
    RESOURCE_ID_IMAGE_LOGO_DRAKE,
    RESOURCE_ID_IMAGE_LOGO_ORIGIN,
    RESOURCE_ID_IMAGE_LOGO_STARCITIZEN,
    RESOURCE_ID_IMAGE_LOGO_FRONTIER,
    RESOURCE_ID_IMAGE_LOGO_HEADHUNTERS,
  };
  const uint8_t logo = watchface_get_logo();
  if (logo >= 1 && logo <= 9) {
    s_logo_bmp = gbitmap_create_with_resource(logo_res[logo - 1]);
  }
}

// Draw the constructor logo — always at the natural bitmap size: the
// assets are pre-baked per platform (flattened on the panel background
// for color displays, binary-alpha silhouettes for B&W), so a plain
// opaque draw is pixel-exact everywhere.
static void prv_draw_logo(GContext *ctx, GRect logo_rect) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_logo_bmp, logo_rect);
}

// Worst-case width of "04:44" per time face (Rajdhani digits scale at
// ~2.29x the point size, measured at 56px) — the time/logo/date row is
// laid out on this fixed width so nothing drifts when digits change.
static int prv_time_max_w(FontSize f) {
  switch (f) {
    case FONT_SIZE_TIME_SMALL:   return 92;   // 40px face
    case FONT_SIZE_TIME:         return 115;  // 50px face
    case FONT_SIZE_TIME_BIG:     return 138;  // 60px face
    case FONT_SIZE_TIME_HUGE:    return 165;  // 72px face
    case FONT_SIZE_TIME_MASSIVE: return 184;  // 80px face
    default:                     return 138;
  }
}

static void prv_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Panel chrome + header label
  panel_draw_header_full(ctx, bounds, "NAVCOMP", COLOR_PRIMARY);

  GRect content = panel_content_rect(bounds);
  const bool date_on = watchface_get_show_date();
  GRect logo_bounds = (s_logo_bmp != NULL) ? gbitmap_get_bounds(s_logo_bmp) : GRectZero;
  // Date height for the block layout: hero rungs may shrink it 18 -> 14px
  // to fit the full logo; flint is fixed at 14px (hero never runs there)
  int date_h_hero = 18;
#if PBL_DISPLAY_WIDTH < 200
  // Flint (144px wide): the 36px logo zone + the 40px face fit side by
  // side (worst "04:44" = 92px in a 92px zone); the date drops to 14px to
  // keep the whole block inside the short content
  const bool hero = false;
  const bool show_logo = (s_logo_bmp != NULL);
  const FontSize time_font = FONT_SIZE_TIME_SMALL;
  const int time_rect_h = 40;
  const int l_time = FONT_LEADING_40;
#else
  const bool show_logo = (s_logo_bmp != NULL);
  // Hero mode = tall content (time-only, or the other rows pinned at their
  // minimum leaves TIME huge): time goes full-width with the logo centered
  // below it; 80px only if width AND the full block (time + logo + date)
  // fit the content, else step down to 72/60px.
  // Hero is reserved for TIME-only and TIME+SYSTEMS layouts: MEDICAL or
  // ENVIRON on screen keeps the logo in its right-side zone — beside the
  // time it renders better than the cramped under-time slot.
  const bool hero = content.size.h >= 120 &&
      !(watchface_get_config() & (CONFIG_MEDICAL | CONFIG_ENVIRON));
  FontSize time_font;
  int time_rect_h;
  if (hero) {
    const int logo_h = show_logo ? 6 + logo_bounds.size.h : 0;
    // Each rung requires the FULL logo below the time (logo_h = 6 + logo
    // height): the logo is never squeezed — the date first shrinks
    // 18 -> 14px, then the face steps down; the final SMALL rung
    // guarantees the block on short contents (gabbro TIME+SYSTEMS)
    const int dp = date_on ? 21 : 0;   // date block at 18px
    const int dps = date_on ? 17 : 0;  // date block at 14px
    if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_MASSIVE) &&
        76 + logo_h + dp <= content.size.h) {
      time_font = FONT_SIZE_TIME_MASSIVE;
      time_rect_h = 76;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_HUGE) &&
               68 + logo_h + dp <= content.size.h) {
      time_font = FONT_SIZE_TIME_HUGE;
      time_rect_h = 68;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_HUGE) &&
               68 + logo_h + dps <= content.size.h) {
      time_font = FONT_SIZE_TIME_HUGE;
      time_rect_h = 68;
      date_h_hero = 14;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_BIG) &&
               56 + logo_h + dp <= content.size.h) {
      time_font = FONT_SIZE_TIME_BIG;
      time_rect_h = 56;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_BIG) &&
               56 + logo_h + dps <= content.size.h) {
      time_font = FONT_SIZE_TIME_BIG;
      time_rect_h = 56;
      date_h_hero = 14;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME) &&
               52 + logo_h + dp <= content.size.h) {
      time_font = FONT_SIZE_TIME;
      time_rect_h = 52;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME) &&
               52 + logo_h + dps <= content.size.h) {
      time_font = FONT_SIZE_TIME;
      time_rect_h = 52;
      date_h_hero = 14;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_SMALL) &&
               40 + logo_h + dp <= content.size.h) {
      time_font = FONT_SIZE_TIME_SMALL;
      time_rect_h = 40;
    } else {
      time_font = FONT_SIZE_TIME_SMALL;
      time_rect_h = 40;
      date_h_hero = 14;
    }
  } else if (show_logo) {
#ifdef PBL_ROUND
    // Gabbro: the chord-narrowed time zone (~97px) cannot fit the 50px
    // face ("04:44" = 115px) — use the 40px face instead
    time_font = FONT_SIZE_TIME_SMALL;
    time_rect_h = 40;
#else
    time_font = FONT_SIZE_TIME;
    time_rect_h = 52;
#endif
  } else if (date_on) {
    // Date takes 21px extra: ladder 72px (block 89) / 60px (77) / 40px (61)
    // / 50px — short chord rows (gabbro) step down instead of clipping
    if (content.size.h >= 89 &&
        content.size.w >= prv_time_max_w(FONT_SIZE_TIME_HUGE)) {
      time_font = FONT_SIZE_TIME_HUGE;
      time_rect_h = 68;
    } else if (content.size.h >= 77 &&
               content.size.w >= prv_time_max_w(FONT_SIZE_TIME_BIG)) {
      time_font = FONT_SIZE_TIME_BIG;
      time_rect_h = 56;
    } else if (content.size.h >= 61 &&
               content.size.w >= prv_time_max_w(FONT_SIZE_TIME_SMALL)) {
      time_font = FONT_SIZE_TIME_SMALL;
      time_rect_h = 40;
    } else {
      time_font = FONT_SIZE_TIME;
      time_rect_h = 52;
    }
  } else {
    // Date hidden: vertical space freed -> bigger time font
    time_font = (content.size.h >= 76 &&
                 content.size.w >= prv_time_max_w(FONT_SIZE_TIME_MASSIVE))
                    ? FONT_SIZE_TIME_MASSIVE
                : (content.size.h >= 68 &&
                   content.size.w >= prv_time_max_w(FONT_SIZE_TIME_HUGE))
                        ? FONT_SIZE_TIME_HUGE
                        : FONT_SIZE_TIME_BIG;
    time_rect_h = (time_font == FONT_SIZE_TIME_MASSIVE) ? 76
                : (time_font == FONT_SIZE_TIME_HUGE) ? 68 : 56;
  }
  const int l_time = (time_font == FONT_SIZE_TIME_MASSIVE) ? FONT_LEADING_80
                   : (time_font == FONT_SIZE_TIME_HUGE) ? FONT_LEADING_72
                   : (time_font == FONT_SIZE_TIME_SMALL) ? FONT_LEADING_40
                   : (time_font == FONT_SIZE_TIME) ? FONT_LEADING_50
                   : FONT_LEADING_60;
#endif

  // Effective logo rect: assets are pre-baked per platform (the flint
  // silhouettes fit the 36px zone), so the natural bitmap size is always
  // the draw size — an oversized asset means a stale file: hide it
  // rather than paint outside the layout
  int logo_draw_w = logo_bounds.size.w;
  int logo_draw_h = logo_bounds.size.h;
#if PBL_DISPLAY_WIDTH < 200
  const int LOGO_ZONE_W = 36;
  if (show_logo &&
      (logo_draw_w > LOGO_ZONE_W || logo_draw_h > content.size.h)) {
    logo_draw_w = 0;
    logo_draw_h = 0;
  }
#endif
  if (hero && show_logo) {
    const int avail =
        content.size.h - time_rect_h - 6 - (date_on ? 3 + date_h_hero : 0) - 2;
    if (avail < logo_draw_h) {
      // pre-baked assets never scale — hide instead of squeezing
      logo_draw_w = 0;
      logo_draw_h = 0;
    }
  }

  // Vertical block: time (+ logo below in hero) (+ date) — centered
  int block_h = time_rect_h;
  if (hero && show_logo && logo_draw_h > 0) block_h += 6 + logo_draw_h;
  if (date_on) block_h += 3 + (PBL_DISPLAY_WIDTH < 200 ? 14 : date_h_hero);
  int y_offset = content.origin.y + (content.size.h - block_h) / 2;
  if (y_offset < content.origin.y) y_offset = content.origin.y;

  // Logo layout (normal mode only): the time slot is sized on the
  // worst-case time width (no per-minute drift); the logo is anchored to
  // the right edge with the SAME margin as the time slot on the left, so
  // it sits centered between the time and the right edge
  int time_x = content.origin.x;
  int time_w = content.size.w;
  int logo_x = content.origin.x;
  if (show_logo && !hero) {
    time_w = prv_time_max_w(time_font);
    int m = (content.size.w - time_w - logo_draw_w) / 3;
    if (m < 0) m = 0;
    time_x = content.origin.x + m;
    // The time stays anchored on its zero-drift worst-case slot, but the
    // digits rarely fill it — center the logo between the VISIBLE digits
    // and the right edge so both gaps read equal whatever the hour
    const int ts_w = graphics_text_layout_get_content_size(
        s_time_buf, fonts_get(time_font), GRect(0, 0, time_w, 80),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter).w;
    const int vis_end = time_x + (time_w + ts_w) / 2;
    logo_x = vis_end +
        (content.origin.x + content.size.w - vis_end - logo_draw_w) / 2;
    // Clamp against the VISIBLE digits, not the reserved slot — the slot
    // tail is empty for narrow hours, the logo may slide into it
    if (logo_x < vis_end) logo_x = vis_end;
    const int logo_max_x = content.origin.x + content.size.w - logo_draw_w;
    if (logo_x > logo_max_x) logo_x = logo_max_x;
  }

  GRect time_rect = GRect(time_x, y_offset - l_time,
                          time_w, time_rect_h + l_time);
  graphics_context_set_text_color(ctx, watchface_get_color_time());
  graphics_draw_text(ctx, s_time_buf, fonts_get(time_font), time_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Cursor below the time: hero draws the logo there, then the date
  int y = y_offset + time_rect_h;
  if (hero && show_logo && logo_draw_h > 0) {
    y += 6;
    GRect logo_rect = GRect(
        content.origin.x + (content.size.w - logo_draw_w) / 2,
        y, logo_draw_w, logo_draw_h);
    prv_draw_logo(ctx, logo_rect);
    y += logo_draw_h;
  }

  if (date_on) {
#if PBL_DISPLAY_WIDTH < 200
    const int date_h = 14;  // flint: 14px date keeps the block inside
    const int l_date = FONT_LEADING_14;
    const FontSize date_font = FONT_SIZE_HEADER;
#else
    const int date_h = date_h_hero;  // 14 = hero shrunk it to fit
    const int l_date = (date_h < 18) ? FONT_LEADING_14 : FONT_LEADING_18;
    const FontSize date_font =
        (date_h < 18) ? FONT_SIZE_HEADER : FONT_SIZE_VALUE;
#endif
    GRect date_rect = GRect(time_x, y + 3 - l_date, time_w,
                            date_h + l_date);
    graphics_context_set_text_color(ctx, watchface_get_color_value());
    graphics_draw_text(ctx, s_date_buf, fonts_get(date_font), date_rect,
                       GTextOverflowModeTrailingEllipsis,
                       GTextAlignmentCenter, NULL);
  }

  // Constructor logo — right side, full-width 64px zone (normal mode only).
  // Vertically + horizontally centered in the fixed right zone.
  if (show_logo && !hero) {
    int logo_y = content.origin.y
               + (content.size.h - logo_draw_h) / 2;
    if (logo_y < content.origin.y) logo_y = content.origin.y;
    prv_draw_logo(ctx, GRect(logo_x, logo_y, logo_draw_w, logo_draw_h));
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

