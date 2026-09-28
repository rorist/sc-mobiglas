# sc-mobiglas — Star Citizen mobiGlas Watchface for Pebble

## Project Overview

**Project root:** this directory IS the git repo root (`sc-mobiglas/`).
Local-only design references (mobiGlas screenshots, wiki PDF, `prompt_initial.md`) live in the
parent `wf-starcitizen/` folder — **outside the repo**, not needed for builds.
**Platforms:** emery — Pebble PT2 200×228 color rectangular (primary) · flint — 144×168 B&W ·
gabbro — 260×260 round (`targetPlatforms` in `package.json`)
**Language:** C (Pebble SDK 3) + PebbleKit JS (PKJS) for phone-side logic
**Description:** Watchface inspired by the Star Citizen mobiGlas holographic UI — panel-based
layout, Rajdhani font, cyan-on-black palette.

> ⚠️ This file is the authoritative project reference. Read it fully before writing any code.
> **Serena memories:** if a `.serena/` directory exists in the CWD, it may be used (project
> memories, notes). Updating memories is **optional** — only when necessary (major milestone or
> architectural change). Not required after every task.

---

## Visual Design

### Color Palette (defined in `watchface.h`)

| Token            | Pebble constant                                 | Exact hex | Usage                                     |
| ---------------- | ----------------------------------------------- | --------- | ----------------------------------------- |
| `COLOR_PRIMARY`  | `GColorVividCerulean`                           | `#00aaff` | Borders, icons, chrome                    |
| `COLOR_GAUGE_BG` | `GColorCobaltBlue`                              | `#0055aa` | Ring gauge tracks                         |
| `COLOR_BG`       | `GColorBlack`                                   | `#000000` | Window background                         |
| `COLOR_PANEL_BG` | `PBL_IF_BW_ELSE(GColorBlack, GColorOxfordBlue)` | `#000055` | Panel fill (black on flint)               |
| `COLOR_WARN`     | `GColorOrange`                                  | `#ff5500` | RAIN/SNOW icons, low battery, HR abnormal |
| `COLOR_SAFE`     | `GColorMalachite`                               | `#00ff55` | Charging battery, HR normal ring          |
| `COLOR_ALERT`    | `GColorRed`                                     | `#ff0000` | STORM icon                                |

Configurable text colors (runtime via `watchface_get_color_*()` getters, defaults in
`watchface.h`, Clay pickers, storage-persisted):
`COLOR_TIME` Celeste `#aaffff` · `COLOR_VALUE` White `#ffffff` · `COLOR_LABEL` PictonBlue
`#55aaff` · `COLOR_HEADER` VividCerulean `#00aaff` · `COLOR_WARN` Orange `#ff8800`
(note: configurable warn color `#ff8800` ≠ system `GColorOrange` `#ff5500`)

**Flint (B&W):** user-configurable colors are clamped to white in `watchface.c` when too dark
(black text on black panels would be invisible).

> Never hardcode hex values — always reference `watchface.h` constants.

### Typography

- **Font:** Rajdhani medium, loaded with `fonts_load_custom_font()` via `fonts_get()` helper
  in `ui/fonts.c`
- **Resources declared in `package.json`:** `FONT_RAJDHANI_14`, `FONT_RAJDHANI_18`,
  `FONT_RAJDHANI_40`, `FONT_RAJDHANI_48`, `FONT_RAJDHANI_50`, `FONT_RAJDHANI_60`
- **FontSize enum (`ui/fonts.h`):**
  - `FONT_SIZE_HEADER` = 14px — panel headers, small text
  - `FONT_SIZE_VALUE` = 18px — panel values, date
  - `FONT_SIZE_TIME` = 50px — HH:MM with logo (flint: 40px)
  - `FONT_SIZE_TIME_BIG` = 60px — HH:MM without logo (flint: 48px)
- **Fallback:** system Gothic 14 / 18 if custom font unavailable
- **Leading compensation (`FONT_LEADING_*`):** large Rajdhani sizes reserve empty space above
  glyphs — text rects are shifted UP by `FONT_LEADING_14=2, FONT_LEADING_18=3,
FONT_LEADING_50/60` (values differ flint vs emery/gabbro) so glyphs align with the intended
  visual top.

> **Font subset:** 14/18 use `characterRegex = "[0-9:A-Za-z /.%°-]"` (degree symbol included);
> 40/48/50/60 are digits only `[0-9: ]`. Unicode symbols (◈ ◉ ⬡ ♥) are **not** in the subset
> and cannot render in Rajdhani — decorative glyphs must be drawn as primitives
> (see icon functions in `ui/draw_utils.c`).

### Panel Aesthetic (mobiGlas)

- **Rounded corners, radius 3:** panel fill via `graphics_fill_rect(..., 3, GCornersAll)` +
  border via `graphics_draw_round_rect(..., 3)` — drawn by `draw_panel_fill()` /
  `draw_panel_border()` in `ui/draw_utils.c` (`draw_corner_accents()` still exists but is
  **not called**)
- Header (drawn by `draw_panel_header()` / `draw_panel_header_ex()`): vertical accent bar +
  left-aligned uppercase label + partial underline + diagonal ↗ arrow glyph at top-right.
  `_ex` variant adds a right-aligned label (e.g. `BAT: 87%` in SYSTEMS header)
- Panel fill: `COLOR_PANEL_BG` (`#000055`, black on flint)
- All coordinates relative to `layer_get_bounds(layer)`; content starts at y+16
  (`panel_content_rect()` in `panels/panel.h`)

### mobiGlas Reference Layout (emery, all panels on)

```
┌─────────────────────────────────┐
│  21:47                 [LOGO]   │
│  WED 01 JUL                     │
├───────────────┬─────────────────┤
│ MEDICAL       │ ENVIRON         │
│   ◎ dual ring │ 12°C  [icon]    │
│  BPM   STEPS  │ 12km/h 62% UV4  │
│               │ ↑06:12 ↓20:44   │
├───────────────┴─────────────────┤
│ SYSTEMS                  BAT 87%│
│ [▓▓▓▓▓▓▓░░░░]                   │
└─────────────────────────────────┘
```

---

## Layout Specification

### Dynamic Layout Engine (`layout.c`)

`layout_compute(screen_bounds, config)` returns a `LayoutInfo` struct (`visible[]`, `rects[]`,
`panel_count`) with a `GRect` per visible panel. Panels toggled via config bitmask bits 2–4.

**Screen dims are platform-aware** — panel widths are derived at runtime from the screen
bounds; only margins/gaps/min heights are fixed. `MARGIN = PBL_IF_ROUND_ELSE(24, 3)`,
`PANEL_GAP = 2`.

#### Panel minimum heights (`watchface.h`, per platform)

| Panel   | emery / gabbro            | flint          |
| ------- | ------------------------- | -------------- |
| TIME    | 88 (60px font)            | 70 (48px font) |
| MEDICAL | 70                        | 58             |
| ENVIRON | 70                        | 58             |
| SYSTEMS | 32 (BAT inline in header) | 24             |

#### Row structure

| Row | Panels                                            | Width       |
| --- | ------------------------------------------------- | ----------- |
| 0   | TIME (always)                                     | full width  |
| 1   | MEDICAL + ENVIRON side-by-side, or one full-width | half / full |
| 2   | SYSTEMS                                           | full width  |

#### Rules

- TIME is always visible, never disabled
- Both MEDICAL + ENVIRON → side-by-side; only one → full-width
- SYSTEMS always at bottom row; surplus height distributed evenly across active rows
- TIME alone → vertically centered on screen
- Each panel's content auto-centers within its dynamic bounds

---

## Config — Clay (phone) → bitmask (watch)

Settings use **Clay for Pebble** (`@rebble/clay`) — page auto-generated from
`src/pkjs/config.js`; Clay handles `showConfiguration` / `webviewclosed`,
persists to phone localStorage and re-sends config on app launch.

**Settings options:** 12h/24h · °C/°F · MEDICAL/ENVIRON/SYSTEMS panel toggles ·
constructor logo picker (9 logos + None) · 5 color pickers + "reset colors" button
(wired by `src/pkjs/custom-clay.js`).

**package.json messageKeys:** `KEY_TEMP:0`, `KEY_WEATHER:1`, `KEY_REQUEST_WEATHER:2`,
`KEY_12H:5`, `KEY_FAHRENHEIT:6`, `KEY_SHOW_MEDICAL:7`, `KEY_SHOW_ENVIRON:8`,
`KEY_SHOW_SYSTEMS:9`, `KEY_WIND_SPEED:10`, `KEY_WIND_DIR:11`, `KEY_HUMIDITY:12`,
`KEY_UV:13`, `KEY_SUNRISE:14`, `KEY_SUNSET:15`, `KEY_LOGO:16`, `KEY_COLOR_TIME:17`,
`KEY_COLOR_VALUE:18`, `KEY_COLOR_LABEL:19`, `KEY_COLOR_HEADER:20`, `KEY_COLOR_WARN:21`.
(No `KEY_CONFIG` bitmask over AppMessage — Clay sends one message per key.)

**C-side bitmask** (single canonical definition, `watchface.h`) — rebuilt from
the individual Clay toggles by `prv_apply_toggle()` in `appmessage.c`:

| Bit | Constant            | Default | Meaning             |
| --- | ------------------- | ------- | ------------------- |
| 0   | `CONFIG_12H`        | off     | 12-hour time format |
| 1   | `CONFIG_FAHRENHEIT` | off     | Temperature in °F   |
| 2   | `CONFIG_MEDICAL`    | on      | Show MEDICAL panel  |
| 3   | `CONFIG_ENVIRON`    | on      | Show ENVIRON panel  |
| 4   | `CONFIG_SYSTEMS`    | on      | Show SYSTEMS panel  |

Default: `CONFIG_MEDICAL | CONFIG_ENVIRON | CONFIG_SYSTEMS` (all panels on, 24h, Celsius).
`watchface_get_config()` is the source of truth. Capabilities:
`["health", "configurable", "location"]` (`configurable` = settings gear in the Pebble app,
`location` = GPS for weather).

---

## Data Per Panel

### TIME

- Line 1: `HH:MM` — 60px centered (50px when a logo is active: fixed 64px logo zone on the
  right edge, time AND date keep the same position whatever logo is chosen; flint: 48px,
  no logo — doesn't fit 144px width)
- Line 2: `DOW DD MON` — 18px, centered, same width as time zone
- Config: 12/24h via `CONFIG_12H`

### MEDICAL

- Header: `MEDICAL`
- **Dual concentric ring gauge** (`draw_dual_ring()`): outer ring = heart rate %, inner ring
  = steps %
  - HR: `health_service_peek_current_value(HealthMetricHeartRateBPM)` mapped 40–180 bpm →
    0–100%. Ring color: `COLOR_SAFE` (green) if 50–100 bpm, else `COLOR_WARN` (orange)
  - Steps: `health_service_sum_today(HealthMetricStepCount)` vs daily-average goal
    (`health_service_sum_averaged(..., HealthServiceTimeScopeDaily)`, fallback 10 000)
- Layout adapts to width: compact (< 120px) = BPM value centered in ring + steps below;
  full = BPM in ring + `BPM` / `STEPS` labels below
- Values fall back to `---` if health data unavailable (ring shows 0%)

### ENVIRON

- Header: `ENVIRON`
- Temperature `{t}°C`/`{t}°F` + 8×8 holo weather icon (via `KEY_TEMP` / `KEY_WEATHER`)
- Wind speed + direction (with NE arrow icon) (via `KEY_WIND_SPEED` / `KEY_WIND_DIR`)
- Humidity % + UV index (with drop/UV icons) (via `KEY_HUMIDITY` / `KEY_UV`)
- Sunrise / sunset `HH:MM` (with sun/moon icons) (via `KEY_SUNRISE` / `KEY_SUNSET`)
- Setters: `environ_panel_set_weather()`, `set_wind()`, `set_humidity_uv()`, `set_sun()`
- All fields fall back to `---` / `UNKNOWN` until AppMessage received

### SYSTEMS

- Header: `SYSTEMS` with right-aligned `BAT: {n}%` (`draw_panel_header_ex()`)
- Battery bar: 6-segment bar via `draw_battery_bar()` in `draw_utils.c`
- Semantic colors: green (`COLOR_SAFE`) while charging, orange (`COLOR_WARN`) at ≤ 20%,
  else `COLOR_PRIMARY`

---

## Data Flow

```
Phone (PKJS)                   Watch (C)
──────────────────────────────────────────
weather.js ──────┐
config.js ───────┼─► index.js ──► AppMessage.send()
(custom-clay) ───┘                        │
                                          ▼
                                appmessage.c (prv_inbox_received)
                                          │
                     ┌────────────────────┼──────────────────────┐
                     ▼                    ▼                      ▼
        environ_panel_set_*()   watchface_update_config()  watchface_set_logo/color_*
                                          │
                                          ▼
                             prv_rebuild_panels() + layout_compute()
                                          │
                                          ▼
                               LayerUpdateProc per panel
```

### AppMessage Keys (defined in `package.json` → `messageKeys`)

| Key                                                          | Type                  | Source         | Description                                                             |
| ------------------------------------------------------------ | --------------------- | -------------- | ----------------------------------------------------------------------- |
| `KEY_TEMP`                                                   | `Int8`                | weather.js     | Temperature (converted °C/°F phone-side)                                |
| `KEY_WEATHER`                                                | `CString`             | weather.js     | Condition: `CLEAR`, `CLOUDY`, `FOG`, `RAIN`, `SNOW`, `STORM`, `UNKNOWN` |
| `KEY_REQUEST_WEATHER`                                        | `Uint8`               | watch → phone  | Watch asks for a weather refresh (every 30 min)                         |
| `KEY_WIND_SPEED`                                             | `Int16`               | weather.js     | Wind speed km/h                                                         |
| `KEY_WIND_DIR`                                               | `Int16`               | weather.js     | Wind direction degrees                                                  |
| `KEY_HUMIDITY`                                               | `Int8`                | weather.js     | Relative humidity %                                                     |
| `KEY_UV`                                                     | `Int8`                | weather.js     | UV index                                                                |
| `KEY_SUNRISE` / `KEY_SUNSET`                                 | `CString`             | weather.js     | `"HH:MM"`, formatted phone-side                                         |
| `KEY_12H`                                                    | `Int32`               | Clay config.js | 12h format toggle (1/0)                                                 |
| `KEY_FAHRENHEIT`                                             | `Int32`               | Clay config.js | °F toggle (1/0)                                                         |
| `KEY_SHOW_MEDICAL` / `KEY_SHOW_ENVIRON` / `KEY_SHOW_SYSTEMS` | `Int32`               | Clay config.js | Panel toggles                                                           |
| `KEY_LOGO`                                                   | `CString` `"0"`–`"9"` | Clay config.js | Constructor logo (9 = None)                                             |
| `KEY_COLOR_*`                                                | `Int32` `0xRRGGBB`    | Clay config.js | Text colors (CString `#rrggbb` tolerated)                               |

AppMessage buffer: minimum 256 bytes (`app_message_open(256, 256, ...)`).

---

## Phone-side (PKJS — `src/pkjs/`)

### weather.js — Open-Meteo (no API key, geolocation via PKJS)

```js
// GET https://api.open-meteo.com/v1/forecast
//   ?latitude={lat}&longitude={lon}
//   &current=temperature_2m,relative_humidity_2m,weather_code,
//            wind_speed_10m,wind_direction_10m,uv_index
//   &daily=sunrise,sunset
// WMO weathercode → label: 0→CLEAR, 1-3→CLOUDY, 45/48→FOG,
// 51-67→RAIN, 71-77→SNOW, 95-99→STORM
// Fetch is watch-driven: index.js responds to KEY_REQUEST_WEATHER (no setInterval)
```

### config.js + custom-clay.js

- Clay configuration array (`module.exports = [...]`) — NO manual HTML
- Clay framework auto-generates the page, handles `showConfiguration` /
  `webviewclosed`, persists to phone localStorage, auto-sends on app launch
- Options: 12/24h, °C/°F, MEDICAL/ENVIRON/SYSTEMS toggles, logo picker,
  5 color pickers, reset-colors button (custom-clay.js wires a pure-UI customFn)

### index.js

- Keep thin: subscribe to `ready` / `appmessage` (weather fetch on `KEY_REQUEST_WEATHER`)
- Delegate all logic to `weather.js` / `config.js`

---

## Architecture Details

### Drawing Pattern

```c
// Every panel: LayerUpdateProc owns GRect bounds
// 1. panel_draw_chrome(ctx, bounds, color) — fill + rounded border (panels/panel.h)
// 2. panel_draw_header_full(...) / panel_draw_header_with_right(...)
// 3. render content directly with graphics_draw_text() / primitives in the update proc
// All coordinates relative to layer_get_bounds(layer)
```

### Key functions in `ui/draw_utils.c`

- `draw_panel_fill()` / `draw_panel_border()` — rounded (r3) panel fill + border
- `draw_corner_accents()` — L-shaped corner marks (**unused**, kept for reference)
- `draw_panel_header()` / `draw_panel_header_ex()` — accent bar + label + partial underline
  - ↗ arrow; `_ex` adds right-aligned label
- `draw_battery_bar()` — 6-segment battery indicator
- `draw_ring_gauge()`, `draw_dual_ring()` — circular gauges (MEDICAL)
- `draw_weather_icon()` — 8×8 holo icons, 7 conditions (CLEAR/CLOUDY/FOG/RAIN/SNOW/STORM/UNKNOWN)
- `draw_sun_icon()`, `draw_drop_icon()`, `draw_wind_icon()`, `draw_uv_icon()` — 8×8 holo icons

> Never add a function to `draw_utils.c` that duplicates logic already in `panel.h`.
> Use Serena `find_symbol` / `find_referencing_symbols` before creating any new helper.

### Timing

- `tick_timer_service_subscribe(MINUTE_UNIT)` — minute ticks update TIME panel (no seconds
  display → no per-second wakeups, battery-friendly)
- MEDICAL: `prv_refresh_health()` called in the panel's update proc (refresh on every redraw;
  health_service_peek/sum are cheap cached reads; no health event subscription, no extra
  wakeups)
- Weather: watch-driven refresh — `watchface_tick` sends `KEY_REQUEST_WEATHER` outbox every
  30 min (`tm_min % 30 == 0`); PKJS `appmessage` listener triggers the Open-Meteo fetch
  (phone-side setInterval unreliable)
- Config / panel toggles / colors / logo: event-driven via AppMessage (no polling)

### Health API (correct function names)

```c
// Heart rate
HealthValue hr = health_service_peek_current_value(HealthMetricHeartRateBPM);

// Steps today
HealthValue steps = health_service_sum_today(HealthMetricStepCount);

// Daily average (for step goal)
HealthValue avg = health_service_sum_averaged(
  HealthMetricStepCount, start, now, HealthServiceTimeScopeDaily);
```

Requires `"health"` in `capabilities` in `package.json` (already set).
Always check `HealthServiceAccessibilityMaskAvailable` before reading. Fallback: `"---"`.

---

## Serena MCP — Tooling (priority over bash/grep/cat/find)

Always use Serena to navigate and edit this project. Do NOT use bash, grep, cat,
find, sed or ls for file operations unless Serena cannot do the task.

| Use case                                             | Tool                                           |
| ---------------------------------------------------- | ---------------------------------------------- |
| List files in a directory                            | `list_dir`                                     |
| Find a file by name/mask                             | `find_file`                                    |
| Read a file                                          | `read_file`                                    |
| File overview (symbols: functions, classes, structs) | `get_symbols_overview`                         |
| Find a symbol (function, variable, struct)           | `find_symbol`                                  |
| Search text/regex pattern in code                    | `search_for_pattern`                           |
| Check all usages before refactoring                  | `find_referencing_symbols`                     |
| Navigate to a symbol's declaration                   | `find_declaration`                             |
| Replace a symbol's body                              | `replace_symbol_body`                          |
| Insert code before/after a symbol                    | `insert_before_symbol` / `insert_after_symbol` |
| Generic replacement (literal or regex)               | `replace_content`                              |
| Rename a symbol across the codebase                  | `rename_symbol`                                |
| Create a new file                                    | `create_text_file`                             |
| Build diagnostics for a file                         | `get_diagnostics_for_file`                     |
| Read/write project memories                          | `read_memory` / `write_memory`                 |

**Rules:**

1. Before creating any new function: `find_symbol` (does it exist?) + `find_referencing_symbols` (who uses what I'm touching?)
2. Before editing a file: `get_symbols_overview` + `read_file`
3. Never duplicate logic already in `panel.h` or `draw_utils.c`
4. Bash is reserved for build/run commands (`pebble build`, `pebble install`, `pebble logs`);
   `pebble build` must ALWAYS run alone — never piped to grep/tail/sed (inspection commands
   run separately)
5. Beware of `replace_content` in regex mode inserting literal `\n` — prefer
   `replace_symbol_body` for whole functions

---

## Development Commands

```bash
pebble clean                        # clean build files
pebble build                        # build for all targetPlatforms
pebble install --emulator emery     # run in emulator
pebble screenshot --emulator emery screenshot.png  # capture emulator screen as PNG
pebble logs                         # stream watch logs
pypkjs                              # local PKJS dev server
```

Build must pass with **0 warnings** from project code.

> **Always run `pebble` commands alone** — never pipe it (`| grep`, `| tail`, `2>&1`).
> Run bundle inspection (`grep -c ... build/pebble-js-app.js`, etc.) in separate commands.

> **Visual validation:** after any visual change, run `pebble screenshot --emulator emery`
> and inspect the PNG to validate the rendering yourself before considering the task done.

---

## Code Conventions

- `main.c` ≤ 80 lines — all logic delegated to dedicated modules
- All panel coordinates relative to `layer_get_bounds(layer)` (never hardcoded screen coords)
- Platform differences via `PBL_IF_ROUND_ELSE` / `PBL_IF_BW_ELSE` / `PBL_DISPLAY_WIDTH` —
  never hardcode emery dimensions
- `index.js` stays thin — business logic lives in `weather.js` / `config.js`
- Fallback `"---"` for any missing data (health, weather)
