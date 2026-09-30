# Changelog — sc-mobiglas

## [0.3.0] — 2026-09-30

### Added

- **Weather cache** — returning to the watchface now shows the last weather data (up to 10 minutes old) instantly, instead of `---` placeholders while re-fetching; also fewer weather API requests

### Changed

- **Auto-sizing time** — the clock automatically grows when fewer panels are enabled: combo layouts like TIME + SYSTEMS now get hero-size time, and the largest font that fits is always chosen
- **Round displays (Pebble Time 2 Round)** — the layout follows the screen shape: panels keep their corners inside the bezel, and a seamless background with thin separator lines replaces boxed panels
- **ENVIRON flow** — metrics are laid out left-aligned and wrap like text on wide panels; narrower panels pack them with measured widths, abbreviating conditions (CLR, RN, …) and wind speed when space is tight
- **B&W displays (Pebble 2 Duo)** — MEDICAL and ENVIRON panels reworked for the smaller 144px screen: health values read below the rings, weather rows are compact (sunrise/sunset hidden when panels share a row), and info icons become small dots to free space for text

### Fixed

- **Round displays** — panel corners, the "NAVCOMP" header and the battery row were clipped by the bezel; the hero clock could also be truncated ("00:…") — the size ladder now guarantees the widest time fits
- **Hero logo** — rendered as a washed-out white blob on color displays; partial transparency is now dithered so the logo stays crisp at any size
- **ENVIRON packing** — the weather icon vanished from the top row when every metric was enabled, the wind arrow was hidden when humidity was disabled, and the weather row stayed compressed in a pair even when a full row was available; icons are now kept and full rows used whenever they fit
- **B&W displays** — low-battery, communication and storm warnings were invisible (they rendered black on black) and gauge tracks were invisible too; both now render in white
- **B&W logo** — a solid white square appeared instead of the logo beside the time

## [0.2.0] — 2026-09-28

### Added

- **Per-panel configurable metrics** — choose which information each panel displays from the watchface settings:
  - **MEDICAL**: pick up to 8 metrics (BPM, steps, sleep, calories, distance, active minutes, resting calories, deep sleep) — up to 4 ring gauges side-by-side in full width, 2 in split layout
  - **ENVIRON**: toggle weather, wind, humidity, UV, sunrise/sunset individually
  - **SYSTEMS**: toggle battery and communication indicators individually
  - Selections are persisted on the watch
- **Date display** — `DOW DD MON` shown under the time
- **Hero mode** — when the TIME panel is alone, the time displays larger

### Changed

- **SYSTEMS panel redesign** — battery percentage and bar on one line, adaptive bar (3–6 segments depending on available width), plus Bluetooth communication status (antenna icon, `COM OK` / `COM ERR`)

### Fixed

- **Constructor logos** — oversized logos overlapped the time and were clipped at the screen edge; all 9 logos re-exported to fit the 64px logo zone
- **Bluetooth icon color** — now follows the label color setting
- **Settings on Android (Rebble)** — multi-select options in the settings page only partially applied on real phones due to a boolean serialization bug in the Android runtime; values are now normalized before being sent to the watch

## [0.1.0] — 2026-09-23

Initial release.

### Added

- Holographic mobiGlas layout: time, weather, health, and battery panels
- Time with 12h/24h formats
- Live weather via Open-Meteo: temperature, conditions, wind, humidity, UV index, sunrise/sunset
- Health: heart rate and daily steps
- 9 manufacturer logos to choose from
- 5 customizable accent colors + reset
- Toggle MEDICAL / ENVIRON / SYSTEMS panels on or off
- Color displays (Pebble Time 2), basic round support (Pebble Time 2 Round), and B&W displays (Pebble 2 Duo)

### Notes

- On B&W watches, colors render as white and the logo is hidden next to the time (screen too narrow).
