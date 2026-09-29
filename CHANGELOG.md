# Changelog — sc-mobiglas

## [0.3.0] — 2026-09-29

### Added

- **Weather cache** — returning to the watchface now shows the last weather data (up to 10 minutes old) instantly, instead of `---` placeholders while re-fetching; also fewer weather API requests

### Changed

- **Round displays (Pebble Time 2 Round)** — the layout now follows the screen shape: panels keep their corners inside the bezel, and the time picks the largest size that fits (up to 80px in hero mode)
- **B&W displays (Pebble 2 Duo)** — MEDICAL and ENVIRON panels reworked for the smaller 144px screen: health values are read below the rings, weather rows are compact (sunrise/sunset hidden when panels share a row), and gauge tracks are now visible

### Fixed

- **Round displays** — panel corners, the "NAVCOMP" header, and the battery row were clipped by the round bezel
- **B&W displays** — ring gauge tracks were invisible (drawn in a color that renders as black on 1-bit screens)

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
