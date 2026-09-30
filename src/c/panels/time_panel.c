#include "time_panel.h"
#include "panel.h"
#include "../watchface.h"

static Layer *s_layer;
static char s_time_buf[6];   // "HH:MM\0"
static char s_date_buf[16];  // "TUE 01 JUL\0"
static GBitmap *s_logo_bmp;
#if !PBL_BW
// A2: cached downscaled copy — the layout rect is deterministic, so the
// scale pass runs once per (logo, size) instead of every frame.
// Color platforms only: 1-bit bitmaps have no transparent entry, so the
// B&W path keeps the per-pixel scaling.
static GBitmap *s_logo_scaled;
static GSize s_logo_scaled_size;  // (0,0) = no cached copy yet
#endif  // !PBL_BW

// (Re)load the constructor logo bitmap matching watchface_get_logo()
static void prv_load_logo(void) {
  if (s_logo_bmp) {
    gbitmap_destroy(s_logo_bmp);
    s_logo_bmp = NULL;
  }
#if !PBL_BW
  // A2: the scaled cache follows the logo — drop it on reload
  if (s_logo_scaled) {
    gbitmap_destroy(s_logo_scaled);
    s_logo_scaled = NULL;
  }
  s_logo_scaled_size = GSizeZero;
#endif  // !PBL_BW
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

// Nearest-neighbor downscale for 8-bit palette bitmaps: hero-mode logos too
// tall for the 80px face. Transparent palette entries (alpha 0) are skipped;
// anything else falls back to a plain draw.
// Bits per pixel: 8 = one palette index per byte; palette formats pack
// 2/4/8 indices per byte (ImageMagick exports decode as 4BitPalette)
static int prv_bpp(GBitmapFormat fmt) {
  switch (fmt) {
    case GBitmapFormat8Bit:        return 8;
    case GBitmapFormat4BitPalette: return 4;
    case GBitmapFormat2BitPalette: return 2;
    case GBitmapFormat1BitPalette: return 1;
    default:                       return 0;
  }
}

// Palette index of pixel sx (Pebble packs high bits first = leftmost pixel)
static uint8_t prv_px_index(const uint8_t *data, int sx, int bpp) {
  switch (bpp) {
    case 8: return data[sx];
    case 4: return (data[sx >> 1] >> ((sx & 1) ? 0 : 4)) & 0xF;
    case 2: return (data[sx >> 2] >> (6 - (sx & 3) * 2)) & 0x3;
    case 1: return (data[sx >> 3] >> (7 - (sx & 7))) & 0x1;
  }
  return 0;
}

static void prv_draw_bitmap_scaled(GContext *ctx, GBitmap *bmp, GRect dst) {
  const int bpp = prv_bpp(gbitmap_get_format(bmp));
  if (bpp == 0 || dst.size.w <= 0 || dst.size.h <= 0) {
    graphics_draw_bitmap_in_rect(ctx, bmp, dst);
    return;
  }
  const GRect src = gbitmap_get_bounds(bmp);
  GColor *palette = gbitmap_get_palette(bmp);
  if (!palette) {
    graphics_draw_bitmap_in_rect(ctx, bmp, dst);
    return;
  }
  const int sw = src.size.w, sh = src.size.h;
  for (int y = 0; y < dst.size.h; y++) {
    const int sy = y * sh / dst.size.h;
    const GBitmapDataRowInfo row_info = gbitmap_get_data_row_info(bmp, sy);
    for (int x = 0; x < dst.size.w; x++) {
      const int sx = x * sw / dst.size.w;
      if (sx < row_info.min_x || sx > row_info.max_x) continue;
      const GColor c = palette[prv_px_index(row_info.data, sx, bpp)];
      if (c.a == 0 || c.a == 1) continue;  // transparent / near-transparent
      graphics_context_set_fill_color(ctx, c);
      graphics_fill_rect(ctx, GRect(dst.origin.x + x, dst.origin.y + y, 1, 1),
                         0, GCornerNone);
    }
  }
}

#if !PBL_BW
// Build the downscaled logo for the A2 cache: nearest-neighbor samples of
// the source palette, transparent entries kept as GColorClear so drawing
// the cached copy reproduces the previous per-pixel skip logic exactly.
static GBitmap *prv_scale_logo(GBitmap *bmp, GSize size) {
  const int bpp = prv_bpp(gbitmap_get_format(bmp));
  GColor *palette = gbitmap_get_palette(bmp);
  if (bpp == 0 || !palette || !gbitmap_get_data(bmp) ||
      size.w <= 0 || size.h <= 0) {
    return NULL;
  }
  const GRect src = gbitmap_get_bounds(bmp);
  const int sw = src.size.w, sh = src.size.h;
  GBitmap *out = gbitmap_create_blank(
      size, PBL_IF_BW_ELSE(GBitmapFormat1Bit, GBitmapFormat8Bit));
  if (!out) {
    return NULL;
  }
  const int stride = gbitmap_get_bytes_per_row(out);
  uint8_t *dst = gbitmap_get_data(out);
  // 0 = GColorClear (8-bit) / white (1-bit): pixels the skip logic leaves
  // untouched stay transparent (or white on B&W) in the cached copy
  memset(dst, 0, stride * size.h);
  for (int y = 0; y < size.h; y++) {
    const int sy = y * sh / size.h;
    const GBitmapDataRowInfo row_info = gbitmap_get_data_row_info(bmp, sy);
    for (int x = 0; x < size.w; x++) {
      const int sx = x * sw / size.w;
      if (sx < row_info.min_x || sx > row_info.max_x) continue;
      const GColor c = palette[prv_px_index(row_info.data, sx, bpp)];
      if (c.a == 0 || c.a == 1) continue;  // transparent / near-transparent
#if PBL_BW
      if (!gcolor_equal(c, GColorWhite)) {
        dst[y * stride + (x >> 3)] |= 0x80 >> (x & 7);
      }
#else
      // Bake full alpha: partial-alpha pixels are kept solid (they are
      // the bulk of the antialiased strokes) — keeping a == 2 would
      // alpha-blend again on draw and wash the logo out, while 50%
      // dithering them read as too transparent.
      dst[y * stride + x] = c.argb | 0xC0;
#endif
    }
  }
  return out;
}
#endif  // !PBL_BW

// Draw the constructor logo. At the natural bitmap height this is a plain
// opaque draw (palette transparency handled inside). Otherwise the logo is
// downscaled: from a cached per-(logo, size) copy on color platforms (A2),
// per-pixel on B&W — 1-bit bitmaps have no transparent entry, so a cached
// copy would paint a white box over the background.
static void prv_draw_logo(GContext *ctx, GRect logo_rect) {
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  if (logo_rect.size.h == gbitmap_get_bounds(s_logo_bmp).size.h) {
    graphics_draw_bitmap_in_rect(ctx, s_logo_bmp, logo_rect);
    return;
  }
#if PBL_BW
  prv_draw_bitmap_scaled(ctx, s_logo_bmp, logo_rect);
#else
  if (!s_logo_scaled || s_logo_scaled_size.w != logo_rect.size.w ||
      s_logo_scaled_size.h != logo_rect.size.h) {
    if (s_logo_scaled) {
      gbitmap_destroy(s_logo_scaled);
    }
    s_logo_scaled = prv_scale_logo(s_logo_bmp, logo_rect.size);
    s_logo_scaled_size = logo_rect.size;
  }
  if (s_logo_scaled) {
    graphics_draw_bitmap_in_rect(ctx, s_logo_scaled, logo_rect);
  } else {
    prv_draw_bitmap_scaled(ctx, s_logo_bmp, logo_rect);  // alloc fallback
  }
#endif  // PBL_BW
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
    // 80px face as soon as the width allows it; each rung requires the
    // FULL logo to fit below the time (logo_h = 6 + logo height), so the
    // logo is never squeezed — the block steps down to 72/60px instead
    if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_MASSIVE) &&
        76 + logo_h + (date_on ? 21 : 0) <= content.size.h) {
      time_font = FONT_SIZE_TIME_MASSIVE;
      time_rect_h = 76;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_HUGE) &&
               68 + logo_h + (date_on ? 21 : 0) <= content.size.h) {
      time_font = FONT_SIZE_TIME_HUGE;
      time_rect_h = 68;
    } else if (content.size.w >= prv_time_max_w(FONT_SIZE_TIME_BIG) &&
               56 + logo_h + (date_on ? 21 : 0) <= content.size.h) {
      time_font = FONT_SIZE_TIME_BIG;
      time_rect_h = 56;
    } else {
      time_font = FONT_SIZE_TIME;
      time_rect_h = 52;
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

  // Logo scale cap: flint downscales logos into its narrow 36px zone
#if PBL_DISPLAY_WIDTH < 200
  const int LOGO_ZONE_W = 36;
#endif
  // Effective logo rect: flint fits logos into the narrow zone; hero
  // downscales into the leftover height under the face (2px cushion) or
  // hides the logo entirely (ratio preserved, nearest-neighbor)
  int logo_draw_w = logo_bounds.size.w;
  int logo_draw_h = logo_bounds.size.h;
#if PBL_DISPLAY_WIDTH < 200
  if (show_logo) {
    if (logo_draw_w > LOGO_ZONE_W) {
      logo_draw_h = logo_draw_h * LOGO_ZONE_W / logo_draw_w;
      logo_draw_w = LOGO_ZONE_W;
    }
    if (logo_draw_h > content.size.h) {
      logo_draw_w = logo_draw_w * content.size.h / logo_draw_h;
      logo_draw_h = content.size.h;
    }
  }
#endif
  if (hero && show_logo) {
    const int avail = content.size.h - time_rect_h - 6 - (date_on ? 21 : 0) - 2;
    if (avail < 20) {
      logo_draw_w = 0;
      logo_draw_h = 0;
    } else if (avail < logo_draw_h) {
      logo_draw_w = logo_draw_w * avail / logo_draw_h;
      logo_draw_h = avail;
    }
  }

  // Vertical block: time (+ logo below in hero) (+ date) — centered
  int block_h = time_rect_h;
  if (hero && show_logo && logo_draw_h > 0) block_h += 6 + logo_draw_h;
  if (date_on) block_h += 3 + (PBL_DISPLAY_WIDTH < 200 ? 14 : 18);
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
    logo_x = content.origin.x + content.size.w - m - logo_draw_w;
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
    const int date_h = 18;
    const int l_date = FONT_LEADING_18;
    const FontSize date_font = FONT_SIZE_VALUE;
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
#if !PBL_BW
  if (s_logo_scaled) {
    gbitmap_destroy(s_logo_scaled);
    s_logo_scaled = NULL;
    s_logo_scaled_size = GSizeZero;
  }
#endif  // !PBL_BW
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

