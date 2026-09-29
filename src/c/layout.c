#include "watchface.h"

// ---------------------------------------------------------------------------
// Dynamic layout engine
//
// Rules:
// - TIME is always visible, always the first row, full-width
// - MEDICAL and ENVIRON sit side-by-side if both visible; otherwise full-width
// - SYSTEMS is always the last row, full-width
// - Available height: non-TIME rows stay pinned at their minimum height,
//   all leftover space goes to TIME (bigger time face) — flint keeps the
//   even split (its tight budget needs it)
// - If only TIME is visible, it is vertically centered
// - Round displays: rows use the bezel chord width + vertical edge inset
// ---------------------------------------------------------------------------

// Panel ordering: rows are laid out top-to-bottom.
// Row 0: TIME (always)
// Row 1: MEDICAL and/or ENVIRON (middle row)
// Row 2: SYSTEMS (bottom)

#ifdef PBL_ROUND
// Round displays (gabbro): rows spanning the full square width would get
// their corners clipped by the bezel. Constrain each row to the horizontal
// chord of the screen circle at its outermost edge, and inset the stack
// vertically so the first row's top edge stays well inside the circle.
#define ROUND_EDGE_INSET_TOP 16
#define ROUND_EDGE_INSET_BOTTOM_HERO 16
#define ROUND_CHORD_PAD 2

// Integer sqrt (Newton bit-shift method), exact for non-negative ints.
// The firmware standard library has no sqrt() — do not use libm here.
static int round_isqrt(int v) {
  if (v <= 0) return 0;
  int res = 0;
  int bit = 1 << 30;
  while (bit > v) bit >>= 2;
  while (bit != 0) {
    if (v >= res + bit) {
      v -= res + bit;
      res = (res >> 1) + bit;
    } else {
      res >>= 1;
    }
    bit >>= 2;
  }
  return res;
}

// Width of the circle chord at the row's outermost edge (top or bottom),
// minus a safety pad, clamped to [0, full_w].
static int round_chord_width(GRect screen, int y0, int y1, int full_w) {
  const int r = screen.size.w / 2;
  const int cy = screen.origin.y + screen.size.h / 2;
  int d0 = y0 - cy;
  if (d0 < 0) d0 = -d0;
  int d1 = y1 - cy;
  if (d1 < 0) d1 = -d1;
  const int d = (d0 > d1) ? d0 : d1;
  if (d >= r) return 0;
  const int half = round_isqrt(r * r - d * d) - ROUND_CHORD_PAD;
  if (half <= 0) return 0;
  int w = 2 * half;
  if (w > full_w) w = full_w;
  return w;
}
#endif  // PBL_ROUND

LayoutInfo layout_compute(GRect screen, uint8_t config) {
  LayoutInfo info;
  memset(&info, 0, sizeof(info));

  // Panel widths derived from actual screen bounds (platform-aware)
  const int full_w = screen.size.w - 2 * MARGIN;

  // TIME is always visible
  info.visible[PANEL_TIME] = true;
  info.visible[PANEL_MEDICAL] = (config & CONFIG_MEDICAL) != 0;
  info.visible[PANEL_ENVIRON] = (config & CONFIG_ENVIRON) != 0;
  info.visible[PANEL_SYSTEMS] = (config & CONFIG_SYSTEMS) != 0;

  // Count visible panels (for panel_count field)
  info.panel_count = 0;
  for (int i = 0; i < PANEL_COUNT; i++) {
    if (info.visible[i]) info.panel_count++;
  }

  // Determine which rows exist
  bool has_mid = info.visible[PANEL_MEDICAL] || info.visible[PANEL_ENVIRON];
  bool has_bot = info.visible[PANEL_SYSTEMS];
  int row_count = 1 + (has_mid ? 1 : 0) + (has_bot ? 1 : 0);

  // Round displays: inset the stack vertically so the outer rows keep their
  // corners inside the bezel (a solo TIME gets symmetric insets).
  int inset_top = 0;
  int inset_bottom = 0;
#ifdef PBL_ROUND
  inset_top = ROUND_EDGE_INSET_TOP;
  if (row_count == 1) inset_bottom = ROUND_EDGE_INSET_BOTTOM_HERO;
#endif

  // Available height for panels
  int avail_h = screen.size.h - 2 * MARGIN - inset_top - inset_bottom
                - (row_count - 1) * PANEL_GAP;

  // Minimum heights for each row
  int min_time = TIME_MIN_H;
  int min_mid  = has_mid ? (info.visible[PANEL_MEDICAL] && info.visible[PANEL_ENVIRON]
                            ? (MEDICAL_MIN_H > ENVIRON_MIN_H ? MEDICAL_MIN_H : ENVIRON_MIN_H)
                            : (info.visible[PANEL_MEDICAL] ? MEDICAL_MIN_H : ENVIRON_MIN_H))
                         : 0;
  int min_bot  = has_bot ? SYSTEMS_MIN_H : 0;

  int total_min = min_time + min_mid + min_bot;
  int surplus = avail_h - total_min;

  // Distribute heights: surplus evenly across rows; on deficit compress
  // proportionally to min heights (defensive, keeps the layout valid).
  int h_time, h_mid, h_bot;

  if (row_count == 1) {
    // Only TIME — give it all the height
    h_time = avail_h;
    h_mid = 0;
    h_bot = 0;
  } else if (surplus >= 0) {
#if PBL_DISPLAY_WIDTH < 200
    // Flint: budget is tight, keep the even split across rows
    int per_row = surplus / row_count;
    int remainder = surplus % row_count;

    h_time = min_time + per_row + (remainder > 0 ? 1 : 0);
    if (remainder > 0) remainder--;

    h_mid = has_mid ? min_mid + per_row + (remainder > 0 ? 1 : 0) : 0;
    if (remainder > 0) remainder--;

    h_bot = has_bot ? min_bot + per_row : 0;
#else
    // Emery/gabbro: data rows pinned at their minimum, all the surplus
    // goes to TIME (SYSTEMS keeps the same height whatever the layout)
    h_time = min_time + surplus;
    h_mid = has_mid ? min_mid : 0;
    h_bot = has_bot ? min_bot : 0;
#endif
  } else {
    int remaining = avail_h;
    h_time = min_time * avail_h / total_min;
    remaining -= h_time;
    h_mid = has_mid ? min_mid * avail_h / total_min : 0;
    remaining -= h_mid;
    h_bot = has_bot ? min_bot * avail_h / total_min : 0;
    remaining -= h_bot;
    if (remaining > 0) h_time += remaining;
  }

  // Compute Y positions
  int y = screen.origin.y + MARGIN + inset_top;

  // If only TIME, center it vertically
  if (row_count == 1) {
    y = screen.origin.y + (screen.size.h - h_time) / 2;
  }

  // Row 0: TIME — width limited to the bezel chord on round displays
  int w_time = full_w;
  int x_time = screen.origin.x + MARGIN;
#ifdef PBL_ROUND
  w_time = round_chord_width(screen, y, y + h_time, full_w);
  x_time += (full_w - w_time) / 2;
#endif
  info.rects[PANEL_TIME] = GRect(x_time, y, w_time, h_time);
  y += h_time + PANEL_GAP;

  // Row 1: MEDICAL / ENVIRON
  if (has_mid) {
    int w_mid = full_w;
    int x_mid = screen.origin.x + MARGIN;
#ifdef PBL_ROUND
    w_mid = round_chord_width(screen, y, y + h_mid, full_w);
    x_mid += (full_w - w_mid) / 2;
#endif
    bool both = info.visible[PANEL_MEDICAL] && info.visible[PANEL_ENVIRON];
    if (both) {
      // Side by side
      int half_w = (w_mid - PANEL_GAP) / 2;
      info.rects[PANEL_MEDICAL] = GRect(x_mid, y, half_w, h_mid);
      info.rects[PANEL_ENVIRON] = GRect(x_mid + half_w + PANEL_GAP, y,
                                        w_mid - half_w - PANEL_GAP, h_mid);
    } else if (info.visible[PANEL_MEDICAL]) {
      // MEDICAL only — full row width
      info.rects[PANEL_MEDICAL] = GRect(x_mid, y, w_mid, h_mid);
    } else {
      // ENVIRON only — full row width
      info.rects[PANEL_ENVIRON] = GRect(x_mid, y, w_mid, h_mid);
    }
    y += h_mid + PANEL_GAP;
  }

  // Row 2: SYSTEMS
  if (has_bot) {
    int w_bot = full_w;
    int x_bot = screen.origin.x + MARGIN;
#ifdef PBL_ROUND
    w_bot = round_chord_width(screen, y, y + h_bot, full_w);
    x_bot += (full_w - w_bot) / 2;
#endif
    info.rects[PANEL_SYSTEMS] = GRect(x_bot, y, w_bot, h_bot);
  }

  return info;
}
