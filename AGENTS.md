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

### Color Palette (`watchface.h`)

| Token            | Definition                                      | Usage                                         |
| ---------------- | ----------------------------------------------- | --------------------------------------------- |
| `COLOR_PRIMARY` | `GColorVividCerulean` `#00aaff`               | Borders, icons, chrome                        |
| `COLOR_GAUGE_BG`| `GColorCobaltBlue` `#0055aa`                  | Ring gauge tracks (color platforms)          |
| `COLOR_BG`      | `GColorBlack`                                 | Window background                             |
| `COLOR_PANEL_BG`| `PBL_IF_BW_ELSE(GColorBlack, GColorOxfordBlue)`| Panel fill (gabbro: global background)        |
| `COLOR_WARN`    | `PBL_IF_BW_ELSE(GColorWhite, GColorOrange)`    | RAIN/SNOW, low battery, HR abnormal, COM ERR  |
| `COLOR_SAFE`    | `PBL_IF_BW_ELSE(GColorWhite, GColorMalachite)` | Charging battery, HR normal                   |
| `COLOR_ALERT`   | `PBL_IF_BW_ELSE(GColorWhite, GColorRed)`       | STORM icon                                    |

On flint (1-bit) orange/red binarize to black on black = invisible → semantic colors are
clamped to white **in `watchface.h`**; the gauge track is handled in `medical_panel.c`
(see Multi-platform).

**Configurable text colors — ColorSlot (table-driven):** enum `COLOR_SLOT_TIME / VALUE /
LABEL / HEADER / WARN` (`COLOR_SLOT_COUNT`), defaults `COLOR_*_DEFAULT`: Celeste
`#aaffff` · White `#ffffff` · PictonBlue `#55aaff` · VividCerulean `#00aaff` ·
Orange `#ff8800`. Access: `watchface_get_color(slot)` / `watchface_set_color(slot, …)`
plus named getters; persisted as argb per slot (`storage_load/save_color`); Clay keys map
1:1 (see Config). flint: dark user colors are clamped to white in `watchface.c`
(`prv_clamp_bw()`, single choke point).

> Never hardcode hex values — always reference `watchface.h` constants.

### Typography

- **Font:** Rajdhani medium, loaded via `fonts_get(FontSize)` in `ui/fonts.c`
- **Resources (package.json — one TTF, subset by size suffix):** 12 / 14 / 18 / 40 / 48 /
  50 / 60 / 72 / 80
- **FontSize enum (`ui/fonts.h`):** `FONT_SIZE_HEADER` 14 · `FONT_SIZE_VALUE` 18 ·
  `FONT_SIZE_TIME` 50 (with logo) · `FONT_SIZE_TIME_BIG` 60 · `FONT_SIZE_TIME_HUGE` 72 ·
  `FONT_SIZE_TIME_MASSIVE` 80 (hero) · `FONT_SIZE_TIME_SMALL` 40 · `FONT_SIZE_METRIC` 12
- **flint loading:** skips the unused 18px face; TIME→40, BIG→48, HUGE/MASSIVE alias→48,
  SMALL→40 — one `fonts_load_custom_font()` per face, deinit dedupes. gabbro also loads
  SMALL (logo mode). Other platforms load all faces.
- **Leading compensation (`FONT_LEADING_*`):** Rajdhani reserves empty space above glyphs —
  text rects are shifted UP by the leading so glyphs align with the intended visual top.
  12=2 · 14=2 · 18=3 · 40=6 (all platforms); 50/60/72/80: flint 6/7/7/7 (HUGE/MASSIVE map to
  the 48px face), emery/gabbro 7/9/11/12.
- **Subsets:** 12/14/18 = `[0-9:A-Za-z /.%°-]`; 40–80 = digits `[0-9: ]`. Unicode symbols
  (◈ ◉ ⬡ ♥) are **not** in the subset — decorative glyphs are drawn as primitives
  (`ui/draw_utils.c`).
- **Fallback:** system Gothic 14/18 if a custom font is unavailable.

### Panel Aesthetic (mobiGlas)

- **Chrome per platform** — `panel_draw_chrome()` (`panels/panel.h`): emery/flint = panel
  fill (`COLOR_PANEL_BG`) + rounded border r3 (`draw_panel_fill()` /
  `draw_panel_border()`); gabbro = no-op (global background + separators, see
  Multi-platform)
- Headers: `draw_panel_header()` / `draw_panel_header_ex()` — accent bar + uppercase label
  + partial underline + ↗ arrow; `_ex` adds a right-aligned label (available, currently
  unused)
- Content rect `panel_content_rect()` (panel.h): x+4, w−8; flint y+16, h−16 / others
  y+18, h−22 (3px below the header underline, 4px bottom pad)
- `draw_corner_accents()` has been **deleted** (dead code, 2026-09-30)

### Reference layout (emery, all panels on)

```
┌─────────────────────────────────┐
│ NAVCOMP    21:47         [LOGO] │
│            WED 01 JUL           │
├───────────────┬─────────────────┤
│ MEDICAL       │ ENVIRON         │
│  ◎ ◎ rings    │ ☁ 12°C ↗ 12km/h │
│  BPM   STEPS  │ 💧 62% · UV 4   │
│               │ ☀ 06:12 ☾ 20:44 │
├───────────────┴─────────────────┤
│ SYSTEMS  BAT 100% ▮▮▮▯▯  COM OK │
└─────────────────────────────────┘
```

---

## Layout Specification

### Dynamic layout engine (`layout.c`)

`layout_compute(screen_bounds, config)` returns a `LayoutInfo` (visible[], rects[],
panel_count). Panels toggle via config bitmask bits 2–4 (PANEL_TIME is always visible).

- **Margins/gaps (`watchface.h`):** flint MARGIN 2 / PANEL_GAP 1; else
  `PBL_IF_ROUND_ELSE(24, 3)` / 2
- **Min heights (`watchface.h`):**

| Panel   | emery / gabbro | flint                      |
| ------- | -------------- | -------------------------- |
| TIME    | 88             | 74 (48px face + 14px date) |
| MEDICAL | 70             | 56                         |
| ENVIRON | 70             | 56                         |
| SYSTEMS | 38 (14px line + pad) | 32                  |

flint mins sum to the exact available height (168 − 2×2 − 2×1 = 162).

- **Rows:** 0 = TIME (full width) · 1 = MEDICAL + ENVIRON side-by-side (solo → full width) ·
  2 = SYSTEMS (bottom)
- **Surplus height:** non-flint → all to TIME up to `TIME_MAX_H` 120 (content 100),
  remainder to the middle row (bigger rings); flint → split evenly across rows;
  deficit → proportional shrink
- **TIME alone** → vertically centered (hero, see Multi-platform)
- **gabbro (round):** chord-aware width per row (`round_chord_width()` — custom integer
  sqrt, firmware ships no libm `sqrt`) + 16px vertical edge insets; rows narrower than full
  width are centered

### Multi-platform rendering (flint / emery / gabbro)

- **Dispatch:** `PBL_DISPLAY_WIDTH` (flint 144 < 200 ≤ emery, gabbro 260) and `PBL_ROUND` /
  `PBL_IF_ROUND_ELSE` / `PBL_IF_BW_ELSE` — never hardcode screen dims
- **Chrome** (`watchface.c`): emery/flint = bordered panels on black; gabbro = global
  `COLOR_PANEL_BG` full-screen background + separator layer (H lines at inter-row gaps, V
  line between MEDICAL/ENVIRON inset 4px) — no boxed look on round
- **Time fonts** (`time_panel.c`): ladder rungs check content height AND width ≥
  `prv_time_max_w(face)` (worst-case "04:44": 92@40 / 115@50 / 138@60 / 165@72 / 184@80).
  No logo: with date → ≥89→72px · ≥77→60px · ≥61→40px SMALL · else 50px; no date →
  80/72/60px. Hero (content ≥ 120 high, TIME-only or TIME+SYSTEMS): table ladder
  {MASSIVE 76, HUGE 68, BIG 56, TIME 52, SMALL 40} — the date shrinks 18→14px before the
  face steps down; the final rung guarantees the block on short contents. With logo
  side-by-side (MEDICAL/ENVIRON on): width-aware ladder {BIG 56, TIME 52, SMALL 40} —
  face kept only if `content.w ≥ max_w(face) + 3 + zone_w` and the block fits in height.
  The logo is NEVER scaled: natural size in a fixed right zone (`LOGO_ZONE_W` 64,
  flint 36). flint: time 40px, date 14px (`FONT_SIZE_HEADER`)
- **Vertical distribution** (`time_panel.c`): the time (+ logo in hero) + date block is
  centered in the content; the date gap grows with free height (`3 + (free − 6) / 2`)
  so time and date breathe evenly (floating date)
- **Logo rendering:** pre-baked per-platform assets (`package.json` `targetPlatforms`,
  same resource name twice): color = cyan logos flattened on OxfordBlue and quantized to
  the official Pebble 64 palette (opaque, AA baked into exact palette colors — pixel-
  deterministic on hardware); flint = white silhouettes max-fit 36px, binary alpha. Zero
  runtime scaling: `prv_draw_logo()` is a plain `graphics_draw_bitmap_in_rect` +
  GCompOpSet everywhere; `prv_scale_logo()`/`prv_draw_bitmap_scaled()` are gone. Zone
  capped 64px (flint 36px); 3-gaps centering anchors the logo right so outer margins stay
  equal whatever the digits. An asset exceeding its zone (stale resource) is hidden, never
  scaled
- **ENVIRON** (`environ_panel.c`): wide (≥ 120px) = left-aligned flow, items measured by
  `prv_item_need_w()` and wrapped (strict width for layout, +5/+7 drawing slack); narrow =
  pair packing with measured splits + k-fulls packer (WEATHER first, then WIND, own rows
  while vertical budget lasts); lone half left-aligned. Degradations measured: condition
  full → abbrev (CLR/CLD/FOG/RN/SNW/STM/N-A) → icon only; wind long → "12km/h" → icon
  dropped. flint: icons → 2px dots (`ENV_ICON_W` 4, `ENV_WIDE_MIN` 34); sunrise/sunset
  dropped side-by-side
- **MEDICAL** (`medical_panel.c`): ring ladder — ring ≥ 52px → value 18px VALUE + label
  14px; ≥ 28px → value 14px + label 12px; < 28px → 2-letter label
  (BP/ST/SL/KC/KM/AC/RK/DP) inside + value below (12px METRIC). Values are measured
  (box 200) and step down before ever clipping. Full mode caps the ring count so each
  cell ≥ 44px. Gauge track on flint:
  `PBL_IF_BW_ELSE(GColorWhite, COLOR_GAUGE_BG)` (#0055aa binarizes to black)
- **SYSTEMS:** solo COM → left-aligned
- **Regression guard:** emery rendering must stay pixel-identical — thresholds are sized so
  emery cells never hit the degraded rungs (verified via captures)

---

## Config — Clay (phone) → watch

Settings use **Clay for Pebble** (`@rebble/clay`) — page auto-generated from
`src/pkjs/config.js`. Sections: **Time** (12h/24h, show date, logo picker 9 logos + None) ·
**Panels** (MEDICAL/ENVIRON/SYSTEMS toggles + 3 checkboxgroups: 8/6/2 metrics) ·
**Weather** (°C/°F) · **Colors** (5 pickers + reset button wired by `custom-clay.js`
customFn).

**C-side config bitmask** (`watchface.h`, single canonical definition): bit 0
`CONFIG_12H` · 1 `CONFIG_FAHRENHEIT` · 2 `CONFIG_MEDICAL` · 3 `CONFIG_ENVIRON` ·
4 `CONFIG_SYSTEMS`; default = all panels on, 24h, °C. Toggles merged by `prv_merge_mask()`
in `appmessage.c` (absent tuple = bit kept). `watchface_get_config()` is the source of
truth. Capabilities: `["health", "configurable", "location"]`.

**Metrics masks** (feature #13): one uint8 per panel — storage keys 9/10/11, defaults
MED `0x03` (BPM+STEPS) / ENV `0x3F` (all 6) / SYS `0x03` (BAT+COM). Bits — MED: BPM,
STEPS, SLEEP, KCAL, DISTANCE, ACTIVE, RKCAL, DEEP_SLEEP; ENV: WEATHER, WIND, HUM, UV,
SUNRISE, SUNSET; SYS: BAT, COM.

**package.json messageKeys (LIST format, 24 entries):** arrays first — `KEY_MED_METRICS[8]`
= 10000 (items 10000–10007), `KEY_ENV_METRICS[6]` = 10008 (10008–10013),
`KEY_SYS_METRICS[2]` = 10014 (10014–10015); singles: `KEY_TEMP` 10016 (Int8) ·
`KEY_WEATHER` 10017 (CString) · `KEY_REQUEST_WEATHER` 10018 (Uint8, watch→phone) ·
`KEY_SHOW_DATE` 10019 · `KEY_12H` 10020 · `KEY_FAHRENHEIT` 10021 ·
`KEY_SHOW_MEDICAL/ENVIRON/SYSTEMS` 10022–10024 · `KEY_WIND_SPEED` 10025 (Int16) ·
`KEY_WIND_DIR` 10026 (Int16) · `KEY_HUMIDITY` 10027 (Int8) · `KEY_UV` 10028 (Int8) ·
`KEY_SUNRISE`/`KEY_SUNSET` 10029/10030 (CString "HH:MM") · `KEY_LOGO` 10031 (CString
"0"–"9") · `KEY_COLOR_TIME/VALUE/LABEL/HEADER/WARN` 10032–10036 (Int32 0xRRGGBB; Clay
"color" sends a packed int, CString "#rrggbb" tolerated).

- C: use the generated `MESSAGE_KEY_*` macros (appinfo.h) — never hand-write key numbers;
  PKJS: `require('message_keys')`
- ⚠️ messageKeys changed → `pebble clean` before build (stale appinfo cache)
- ⚠️ Rebble Android serializes raw JS booleans badly in AppMessage arrays — `index.js`
  wraps `Pebble.sendAppMessage` to normalize true/false → 1/0 (permanent workaround)
- AppMessage buffer: `app_message_open(512, 64)` (inbox = Clay payload, outbox = weather
  request); callbacks registered before open

---

## Data Per Panel

### TIME (`time_panel.c`)

- Header `NAVCOMP`; line 1 `HH:MM`, line 2 `DOW DD MON` (18px VALUE; flint 14px HEADER)
- Font ladder, hero gate, logo centering/cache: see Multi-platform
- 12h mode strips the leading zero

### MEDICAL (`medical_panel.c`)

- 8 metrics, table-driven `MedSlot` (label, 2-letter short, value, pct, fill fn): BPM,
  STEPS, SLEEP, KCAL, DISTANCE, ACTIVE, RKCAL, DEEP_SLEEP
- Goals via `health_service_sum_averaged(…, HealthServiceTimeScopeDaily)`, fallbacks:
  10k steps · 8h sleep · 500 kcal · 5 km · 1h active · 1500 rkcal · 2h deep
- HR 40–180 bpm → 0–100%, ring `COLOR_SAFE` 50–100 bpm else WARN
- Layout: full (≥ 120px) up to 4 rings (cell ≥ 44px), compact 2 rings; ring tiers: see
  Multi-platform. Values `---` when health data unavailable
- Refresh: `prv_refresh_health()` at the top of the update proc + dirty each minute (no
  health event subscription, no extra wakeups)

### ENVIRON (`environ_panel.c`)

- 6 metrics: WEATHER / WIND (full) · HUM / UV / SUNRISE / SUNSET (half)
- Layout: flow / pair packing / degradations: see Multi-platform
- State survives panel rebuilds: `s_cond_raw` + `s_last_temp_c` statics (temp + condition
  must arrive in the SAME AppMessage)
- Setters: `environ_panel_set_weather()`, `set_wind()`, `set_humidity_uv()`, `set_sun()`;
  all fields fall back to `---` / `N/A`

### SYSTEMS (`systems_panel.c`)

- Header `SYSTEMS`; content line: `BAT {n}%` left (VALUE color; SAFE charging / WARN ≤ 20%)
  + adaptive battery bar (3–6 segments, min 20px) + `COM OK/ERR` right (VALUE / WARN;
  `draw_comm_icon()`, 2px dot on flint)
- Solo COM → left-aligned; no metrics → header only
- Battery via `battery_state_service_peek()`, Bluetooth via
  `bluetooth_connection_service_peek()`

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
```

- Fetch is watch-driven: `index.js` answers `KEY_REQUEST_WEATHER` (no setInterval —
  phone-side timers are unreliable on Android)
- **Cache:** localStorage `sc-weather-cache`, TTL 10 min — `fetch()` is cache-first: a
  relaunch shows the last weather instantly (no `---` flash) and saves API calls

### config.js + custom-clay.js

- Clay configuration array (`module.exports`), NO manual HTML; Clay auto-generates the
  page, handles `showConfiguration`/`webviewclosed`, persists to phone localStorage,
  auto-sends on app launch
- Reset-colors button = pure UI (no messageKey) + `custom-clay.js` customFn resets the 5
  pickers to defaults on click (user clicks reset then Save)

### index.js

- Keep thin: `new Clay(clayConfig, customFn)` + `ready` / `appmessage` listeners
- `sendAppMessage` wrapper normalizes booleans (Android workaround, see Config)

---

## Architecture Details

### Drawing Pattern

```c
// Every panel: LayerUpdateProc owns GRect bounds
// 1. panel_draw_chrome(ctx, bounds, color) — per-platform fill + border (panel.h)
// 2. panel_draw_header_full(...) (panel.h inline helpers)
// 3. render content with graphics_draw_text() / primitives in the update proc
// All coordinates relative to layer_get_bounds(layer)
```

### Key functions in `ui/draw_utils.c`

- `draw_panel_fill()` / `draw_panel_border()` — rounded (r3)
- `draw_panel_header()` / `draw_panel_header_ex()` — accent bar + label + underline + ↗
- `draw_battery_bar(ctx, bounds, percent, color)` — adaptive 3–6 segments
- `draw_ring_gauge(ctx, box, percent, thickness, track, fill)` — circular gauge (MEDICAL)
- `draw_weather_icon(ctx, origin, cond_idx)` — 7×7 holo; severity: RAIN/SNOW WARN ·
  STORM ALERT · else label color
- `draw_sun_icon()`, `draw_drop_icon()`, `draw_wind_icon()`, `draw_uv_icon()` — 7×7 holo
- `draw_comm_icon(ctx, origin)` — antenna, stroke color set by the caller

> Never add a function to `draw_utils.c` that duplicates logic already in `panel.h`.
> Use `find_symbol` / `find_referencing_symbols` before creating any new helper.

### Timing

- `tick_timer_service_subscribe(MINUTE_UNIT)` — minute ticks update TIME (no seconds → no
  per-second wakeups)
- MEDICAL: `prv_refresh_health()` in the update proc (cheap cached reads) + dirty each
  minute; no health event subscription
- Weather: watch-driven — `watchface_tick` sends `KEY_REQUEST_WEATHER` every 30 min
  (`tm_min % 30 == 0`); PKJS `appmessage` listener triggers the Open-Meteo fetch
- Config / toggles / colors / logo: event-driven via AppMessage (no polling)

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

Requires `"health"` in `capabilities` (already set). Always check
`HealthServiceAccessibilityMaskAvailable` before reading. Fallback: `"---"`.

---

## Serena MCP — Tooling

Use Serena tools (via `execute` → `tools.serena.*`) for navigation and edits; bash is
reserved for build/run commands (`pebble build` — ALWAYS alone, never piped).

Key tools: `read_file` · `list_dir` · `find_file` · `get_symbols_overview` ·
`find_symbol` (arg = `name_path_pattern`) · `search_for_pattern` ·
`find_referencing_symbols` · `replace_content` (literal needles; repl = real newlines) ·
`replace_symbol_body` (⚠️ swallows the signature) · `create_text_file` ·
`read_memory` / `write_memory` / `edit_memory`.

Rules: `find_symbol` before creating any helper; never duplicate logic already in
`panel.h` or `draw_utils.c`; after a partially-failed edit batch, re-read the file
before continuing.

---

## Development Commands

```bash
pebble clean                        # clean build files
pebble build                        # build for all targetPlatforms
pebble install --emulator emery     # run in emulator
pebble screenshot --no-open --emulator emery screenshot.png  # capture emulator screen as PNG
pebble logs                         # stream watch logs
pypkjs                              # local PKJS dev server
./debug.sh --list                   # 35-case debug harness (med-8w = worst-case health)
./debug.sh --emu flint --install all  # same harness on flint/gabbro (captures in debug/<emu>/)
```

Build must pass with **0 warnings** from project code.

> **Always run `pebble` commands alone** — never pipe it (`| grep`, `| tail`, `2>&1`).
> Run bundle inspection (`grep -c ... build/pebble-js-app.js`, etc.) in separate commands.

> **Visual validation:** after any visual change, run `pebble screenshot --no-open --emulator emery`
> and inspect the PNG to validate the rendering yourself before considering the task done.

---

## Code Conventions

- `main.c` ≤ 80 lines — all logic delegated to dedicated modules
- All panel coordinates relative to `layer_get_bounds(layer)` (never hardcoded screen coords)
- Platform differences via `PBL_IF_ROUND_ELSE` / `PBL_IF_BW_ELSE` / `PBL_DISPLAY_WIDTH` —
  never hardcode emery dimensions
- `index.js` stays thin — business logic lives in `weather.js` / `config.js`
- Fallback `"---"` for any missing data (health, weather)

---

## Assistant Behavior

- Keep responses concise — short answers, no restating known context
- Never run `git commit` or `git push` — the user commits himself; always
  propose a one-line commit message instead
- Reply in the user's language (French if the user writes French),
  but always reason in English
