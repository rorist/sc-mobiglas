#include "watchface.h"

// ---------------------------------------------------------------------------
// Dynamic layout engine
//
// Rules:
// - TIME is always visible, always the first row, full-width
// - MEDICAL and ENVIRON sit side-by-side if both visible; otherwise full-width
// - SYSTEMS is always the last row, full-width
// - Available height is distributed proportionally to min heights,
//   with leftover space shared equally
// - If only TIME is visible, it is vertically centered
// ---------------------------------------------------------------------------

// Panel ordering: rows are laid out top-to-bottom.
// Row 0: TIME (always)
// Row 1: MEDICAL and/or ENVIRON (middle row)
// Row 2: SYSTEMS (bottom)

LayoutInfo layout_compute(GRect screen, uint8_t config) {
  LayoutInfo info;
  memset(&info, 0, sizeof(info));

  // Panel widths derived from actual screen bounds (platform-aware)
  const int full_w = screen.size.w - 2 * MARGIN;
  const int half_w = (full_w - PANEL_GAP) / 2;

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

  // Available height for panels
  int avail_h = screen.size.h - 2 * MARGIN - (row_count - 1) * PANEL_GAP;

  // Minimum heights for each row
  int min_time = TIME_MIN_H;
  int min_mid  = has_mid ? (info.visible[PANEL_MEDICAL] && info.visible[PANEL_ENVIRON]
                            ? (MEDICAL_MIN_H > ENVIRON_MIN_H ? MEDICAL_MIN_H : ENVIRON_MIN_H)
                            : (info.visible[PANEL_MEDICAL] ? MEDICAL_MIN_H : ENVIRON_MIN_H))
                         : 0;
  int min_bot  = has_bot ? SYSTEMS_MIN_H : 0;

  int total_min = min_time + min_mid + min_bot;
  int surplus = avail_h - total_min;
  if (surplus < 0) surplus = 0;

  // Distribute surplus proportionally to min heights
  int h_time, h_mid, h_bot;

  if (row_count == 1) {
    // Only TIME — give it all the height
    h_time = avail_h;
    h_mid = 0;
    h_bot = 0;
  } else {
    // Distribute surplus evenly across rows
    int per_row = surplus / row_count;
    int remainder = surplus % row_count;

    h_time = min_time + per_row + (remainder > 0 ? 1 : 0);
    if (remainder > 0) remainder--;

    h_mid = has_mid ? min_mid + per_row + (remainder > 0 ? 1 : 0) : 0;
    if (remainder > 0) remainder--;

    h_bot = has_bot ? min_bot + per_row : 0;
  }

  // Compute Y positions
  int y = screen.origin.y + MARGIN;

  // If only TIME, center it vertically
  if (row_count == 1) {
    int center_y = screen.origin.y + (screen.size.h - h_time) / 2;
    y = center_y;
  }

  // Row 0: TIME
  info.rects[PANEL_TIME] = GRect(screen.origin.x + MARGIN, y, full_w, h_time);
  y += h_time + PANEL_GAP;

  // Row 1: MEDICAL / ENVIRON
  if (has_mid) {
    bool both = info.visible[PANEL_MEDICAL] && info.visible[PANEL_ENVIRON];
    if (both) {
      // Side by side
      info.rects[PANEL_MEDICAL] = GRect(screen.origin.x + MARGIN, y,
                                         half_w, h_mid);
      info.rects[PANEL_ENVIRON] = GRect(screen.origin.x + MARGIN + half_w + PANEL_GAP, y,
                                         half_w, h_mid);
    } else if (info.visible[PANEL_MEDICAL]) {
      // MEDICAL only — full width
      info.rects[PANEL_MEDICAL] = GRect(screen.origin.x + MARGIN, y,
                                         full_w, h_mid);
    } else {
      // ENVIRON only — full width
      info.rects[PANEL_ENVIRON] = GRect(screen.origin.x + MARGIN, y,
                                         full_w, h_mid);
    }
    y += h_mid + PANEL_GAP;
  }

  // Row 2: SYSTEMS
  if (has_bot) {
    info.rects[PANEL_SYSTEMS] = GRect(screen.origin.x + MARGIN, y,
                                       full_w, h_bot);
  }

  return info;
}
