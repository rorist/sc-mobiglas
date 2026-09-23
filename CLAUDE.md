# sc-mobiglas — Star Citizen mobiGlas Watchface for Pebble PT2

## Project Overview

**App directory:** `sc-mobiglas/` (workspace root: `wf-starcitizen/`)
**Platform:** Pebble PT2 (Repebble / Emery) — 200×228px, color, rectangular
**Language:** C (Pebble SDK 3) + PebbleKit JS (PKJS) for phone-side logic
**Description:** Watchface inspired by the Star Citizen mobiGLas holographic UI — panel-based layout, Rajdhani font, cyan-on-black palette.

> ⚠️ This file is the authoritative project reference. Read it fully before writing any code.
> Current implementation state: `.serena/memories/project-state.md` — update after every milestone.
> TODO list: `.serena/memories/todo-list.md` — update after every completed task.

---

## Visual Design

### Color Palette (defined in `watchface.h`)

| Token | Pebble constant | Exact hex | Usage |
|---|---|---|---|
| `COLOR_PRIMARY` | `GColorVividCerulean` | `#00aaff` | Borders, icons, chrome |
| `COLOR_GAUGE_BG` | `GColorCobaltBlue` | `#0055ff` | Ring gauge tracks |
| `COLOR_BG` | `GColorBlack` | `#000000` | Window background |
| `COLOR_PANEL_BG` | `GColorOxfordBlue` | `#001133` | Panel fill |
| `COLOR_WARN` | `GColorOrange` | `#ff5500` | RAIN/SNOW icons, low battery, HR abnormal |
| `COLOR_SAFE` | `GColorMalachite` | `#00aa00` | Charging battery, HR normal ring |
| `COLOR_ALERT` | `GColorRed` | `#ff0000` | STORM icon |

Configurable text colors (runtime, defaults in `watchface.h`, Clay pickers, storage-persisted):
`COLOR_TIME` Celeste `#aaffff` · `COLOR_VALUE` White `#ffffff` · `COLOR_LABEL` PictonBlue `#55aaff` · `COLOR_HEADER` VividCerulean `#00aaff` · `COLOR_WARN` Orange `#ff8800`

> Never hardcode hex values — always reference `watchface.h` constants.

### Typography

- **Font:** Rajdhani medium, loaded with `fonts_load_custom_font()` via `fonts_get()` helper in `ui/fonts.c`
- **Resources declared in `package.json`:** `FONT_RAJDHANI_14`, `FONT_RAJDHANI_18`, `FONT_RAJDHANI_48`
- **Fallback:** system `Gothic 14` / `Gothic 18` if custom font unavailable
- **Size conventions (actual):**
  - Time HH:MM: 48px
  - Panel values / date: 14px
  - Panel headers: 18px (drawn by `draw_panel_header()`)
  - Seconds: 14px (optional via config)

> **Font subset:** `characterRegex = "[0-9:A-Za-z /.%°-]"` — Unicode symbols (◈ ◉ ⬡ ♥)
> are **not** in the subset and cannot render in Rajdhani. Panel headers use text-only labels
> (`MEDICAL`, `ENVIRON`, etc.); decorative glyphs must be drawn as GPath primitives.

### Panel Aesthetic (mobiGlas)

- 1px borders + L-shaped corner accents (4px legs) — drawn by `draw_panel_border()` / `draw_corner_accents()` in `ui/draw_utils.c`
- Header: left-aligned label with 1px underline — drawn by `draw_panel_header()`
- Panel fill: `COLOR_PANEL_BG` (near-black blue)
- 2px inner padding; all coordinates relative to `layer_get_bounds(layer)`

### mobiGLas Reference Layout
```
┌─────────────────────────┐
│  TIME           [date]  │
├───────────┬─────────────┤
│  MEDICAL  │  ENVIRON    │
│  HR: --   │  --°C       │
│  STEPS:-- │  [cond]     │
│           │  EVT: --    │
├───────────┴─────────────┤
│  SYSTEMS       BAT: [█] │
└─────────────────────────┘
```

---

## Layout Specification

### Dynamic Layout Engine (`layout.c`)

`layout_compute(screen_bounds, config)` returns a `LayoutInfo` struct with `GRect` per visible panel.
Panels toggled via config bitmask bits 2–4.

**Screen:** Emery — 200×228px. Margin: 3px. Panel gap: 2px. Full width: 194px. Half width: 96px.

#### Row structure

| Row | Panels | Width | Min height |
|---|---|---|---|
| 0 | TIME (always) | 194px | 78px |
| 1 | MEDICAL + ENVIRON (side-by-side) or one full-width | 194px / 96px each | 70px |
| 2 | SYSTEMS | 194px | 44px |

#### Rules
- TIME is always visible, never disabled
- Both MEDICAL + ENVIRON → side-by-side (96px each)
- Only one of MEDICAL/ENVIRON → full-width (194px)
- SYSTEMS always at bottom row
- Surplus height distributed evenly across active rows
- TIME alone → vertically centered on screen
- Each panel's content auto-centers within its dynamic bounds

---

## Config — Clay (phone) → bitmask (watch)

Settings use **Clay for Pebble** (`@rebble/clay`) — page auto-generated from
`src/pkjs/config.js`; Clay handles `showConfiguration` / `webviewclosed`,
persists to phone localStorage and re-sends config on app launch.

**package.json messageKeys:** `KEY_TEMP:0`, `KEY_WEATHER:1`, `KEY_EVENT_TITLE:2`,
`KEY_EVENT_TIME:3`, `KEY_12H:5`, `KEY_FAHRENHEIT:6`, `KEY_SHOW_MEDICAL:7`,
`KEY_SHOW_ENVIRON:8`, `KEY_SHOW_SYSTEMS:9`, `KEY_SECONDS:10`.
(No `KEY_CONFIG` bitmask over AppMessage — Clay sends one Int32 toggle per key.)

**C-side bitmask** (single canonical definition, `watchface.h`) — rebuilt from
the individual Clay toggles by `prv_apply_toggle()` in `appmessage.c`:

| Bit | Constant | Default | Meaning |
|---|---|---|---|
| 0 | `CONFIG_12H` | off | 12-hour time format |
| 1 | `CONFIG_FAHRENHEIT` | off | Temperature in °F |
| 2 | `CONFIG_MEDICAL` | on | Show MEDICAL panel |
| 3 | `CONFIG_ENVIRON` | on | Show ENVIRON panel |
| 4 | `CONFIG_SYSTEMS` | on | Show SYSTEMS panel |

Default: `CONFIG_MEDICAL | CONFIG_ENVIRON | CONFIG_SYSTEMS` (all panels on, 24h, Celsius).
`watchface_get_config()` is the source of truth. Capabilities: `["health", "configurable"]`
(`configurable` = settings gear in the Pebble app).

---

## Data Per Panel

### TIME
- Line 1: `HH:MM` — 48px Rajdhani, centered
- Line 2: `DOW DD MON` — 14px, centered (e.g. `TUE 01 JUL`)
- Config: 12/24h via `CONFIG_12H`

### MEDICAL
- Header: `MEDICAL`
- Line 1: `HR: {value} BPM` — heart rate via `health_service_peek_current_value(HealthMetricHeartRateBPM)`
- Line 2: `STEPS: {value}` — steps via `health_service_sum_today(HealthMetricStepCount)`
- Both fall back to `---` if health data unavailable
- Goal for step % bar: `health_service_sum_averaged(HealthMetricStepCount, ..., HealthServiceTimeScopeDaily)`, fallback 10 000

### ENVIRON
- Header: `ENVIRON`
- Line 1: `{temp}°C` / `{temp}°F` (via AppMessage `KEY_TEMP`)
- Line 2: `{condition}` (e.g. `SUNNY`, `CLOUDY`, `RAIN`) (via AppMessage `KEY_WEATHER`)
- Line 3: `EVT: {title}` — truncated to 18 chars (via AppMessage `KEY_EVENT_TITLE`)
- Line 4: `{HH:MM}` — event time formatted from Unix epoch (via AppMessage `KEY_EVENT_TIME`)
- All fields fall back to `---` / `UNKNOWN` until AppMessage received

### SYSTEMS
- Header: `SYSTEMS`
- Battery bar: 6-segment filled rects via `draw_battery_bar()` in `draw_utils.c`
- Text: `BAT: {n}%`

---

## Data Flow

```
Phone (PKJS)                   Watch (C)
─────────────────────────────────────────
weather.js ──────┐
calendar.js ─────┼─► index.js ──► AppMessage.send()
config.js ───────┘                       │
                                         ▼
                               appmessage.c (handler)
                                         │
                                         ▼
                               watchface_update_config()
                               + panel data buffers updated
                                         │
                                         ▼
                               LayerUpdateProc per panel
```

### AppMessage Keys (defined in `package.json` → `messageKeys`)

| Key | Type | Source | Description |
|---|---|---|---|
| `KEY_TEMP` | `Int8` | weather.js | Temperature (°C or °F per config) |
| `KEY_WEATHER` | `CString` | weather.js | Condition: `SUNNY`, `CLOUDY`, `RAIN`, `SNOW`, `FOG` |
| `KEY_EVENT_TITLE` | `CString` | calendar.js | Next event title, max 18 chars |
| `KEY_EVENT_TIME` | `Int32` | calendar.js | Unix epoch of next event |
| `KEY_12H` | `Int32` | Clay config.js | 12h format toggle (1/0) |
| `KEY_FAHRENHEIT` | `Int32` | Clay config.js | °F toggle (1/0) |
| `KEY_SHOW_MEDICAL` | `Int32` | Clay config.js | MEDICAL panel toggle |
| `KEY_SHOW_ENVIRON` | `Int32` | Clay config.js | ENVIRON panel toggle |
| `KEY_SHOW_SYSTEMS` | `Int32` | Clay config.js | SYSTEMS panel toggle |
| `KEY_SECONDS` | `Int32` | Clay config.js | Seconds display toggle |

AppMessage buffer: minimum 256 bytes (`app_message_open(256, 256, ...)`).

---

## Phone-side (PKJS — `src/pkjs/`)

### weather.js — Open-Meteo (no API key)
```js
// GET https://api.open-meteo.com/v1/forecast
//   ?latitude={lat}&longitude={lon}&current_weather=true
// Response: { current_weather: { temperature, weathercode } }
// WMO weathercode → label: 0→CLEAR, 1-3→CLOUDY, 51-67→RAIN, 71-77→SNOW, 45/48→FOG
```

### calendar.js
- Read next calendar event from phone via Pebble PKJS
- Truncate title to 18 chars; send Unix epoch for time formatting on watch
- V1 may use a placeholder if phone calendar API is unavailable

### config.js
- Clay configuration array (`module.exports = [...]`) — NO manual HTML
- Clay framework auto-generates the page, handles `showConfiguration` /
  `webviewclosed`, persists to phone localStorage, auto-sends on app launch
- Options: 12/24h, °C/°F, MEDICAL/ENVIRON/SYSTEMS toggles, seconds

### index.js
- Keep thin: subscribe to `ready` / `showConfiguration` / `webviewclosed` / `appmessage`
- Delegate all logic to `weather.js`, `calendar.js`, `config.js`

---

## Architecture Details

### Drawing Pattern
```c
// Every panel: LayerUpdateProc owns GRect bounds
// 1. draw_panel_border(ctx, bounds, COLOR_PRIMARY)
// 2. draw_corner_accents(ctx, bounds, COLOR_PRIMARY)
// 3. draw_panel_header(ctx, bounds, "LABEL", COLOR_PRIMARY, font)
// 4. render content via GTextLayer children
// All coordinates relative to layer_get_bounds(layer)
```

### Key functions in `ui/draw_utils.c`
- `draw_panel_border()` — 1px rect border
- `draw_corner_accents()` — L-shaped 4px corner marks
- `draw_panel_header()` — label + 1px underline
- `draw_battery_bar()` — 6-segment battery indicator
- `draw_ring_gauge()`, `draw_dual_ring()` — circular gauges (MEDICAL)

> Never add a function to `draw_utils.c` that duplicates logic already in `panel.h`.
> Use Serena `find_symbol` / `find_referencing_symbols` before creating any new helper.

### Timing
- `tick_timer_service_subscribe(MINUTE_UNIT)` — minute ticks update TIME panel (no seconds display → no per-second wakeups, battery-friendly)
- MEDICAL: `prv_refresh_health()` called in the panel's update proc + layer marked dirty every minute in `watchface_tick` (health_service_peek/sum are cheap cached reads; no health event subscription, no extra wakeups)
- Weather: watch-driven refresh — `watchface_tick` sends `KEY_REQUEST_WEATHER` outbox every 30 min (`tm_min % 30 == 0`); PKJS `appmessage` listener triggers the Open-Meteo fetch (phone-side setInterval unreliable)
- Config / panel toggles: event-driven via AppMessage (no polling)

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

| Use case | Tool |
|---|---|
| List files in a directory | `list_dir` |
| Find a file by name/mask | `find_file` |
| Read a file | `read_file` |
| File overview (symbols: functions, classes, structs) | `get_symbols_overview` |
| Find a symbol (function, variable, struct) | `find_symbol` |
| Search text/regex pattern in code | `search_for_pattern` |
| Check all usages before refactoring | `find_referencing_symbols` |
| Navigate to a symbol's declaration | `find_declaration` |
| Replace a symbol's body | `replace_symbol_body` |
| Insert code before/after a symbol | `insert_before_symbol` / `insert_after_symbol` |
| Generic replacement (literal or regex) | `replace_content` |
| Rename a symbol across the codebase | `rename_symbol` |
| Create a new file | `create_text_file` |
| Build diagnostics for a file | `get_diagnostics_for_file` |
| Read/write project memories | `read_memory` / `write_memory` |

**Rules:**
1. Before creating any new function: `find_symbol` (does it exist?) + `find_referencing_symbols` (who uses what I'm touching?)
2. Before editing a file: `get_symbols_overview` + `read_file`
3. Never duplicate logic already in `panel.h` or `draw_utils.c`
4. Bash is reserved for build/run commands (`pebble build`, `pebble install`, `pebble logs`)

---

## Development Commands

```bash
pebble build                        # build for emery
pebble install --emulator emery     # run in emulator
pebble screenshot --emulator emery screenshot.png  # capture emulator screen as PNG
pebble logs                         # stream watch logs
pypkjs                              # local PKJS dev server
```

Build must pass with **0 warnings** from project code.

> **Visual validation:** after any visual change, run `pebble screenshot --emulator emery`
> and inspect the PNG to validate the rendering yourself before considering the task done.

---

## Code Conventions

- `main.c` ≤ 80 lines — all logic delegated to dedicated modules
- All panel coordinates relative to `layer_get_bounds(layer)` (never hardcoded screen coords)
- `index.js` stays thin — business logic lives in `weather.js` / `calendar.js` / `config.js`
- Fallback `"---"` for any missing data (health, weather, calendar)
