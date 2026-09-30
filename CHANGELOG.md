# Changelog: sc-mobiglas

## [0.3.0] - 2026-09-30

Full platform support: the round **Pebble Round 2** and the black & white **Pebble 2 Duo** now each get a layout tailored to their screen. The clock auto-sizes to the space it gets and weather is cached for an instant return to the watchface.

### Added

- **Weather cache**: returning to the watchface shows the last weather instantly, up to 10 minutes old.

### Changed

- **Auto-sizing time**: the clock grows when fewer panels are enabled, always using the largest font that fits.
- **Round displays**: the layout follows the screen shape with a seamless background.
- **ENVIRON flow**: metrics wrap like text on wide panels and pack tightly on narrow ones.
- **B&W displays**: MEDICAL and ENVIRON are reworked for the 144px screen.
- **B&W logo**: the logo now fits beside the time as a clean white silhouette.

### Fixed

- **Round displays**: clipped corners, header and battery row are back inside the bezel.
- **Hero clock**: the time can no longer be truncated.
- **Logos**: pre-baked per-platform assets render crisp and centered on every display.
- **ENVIRON packing**: icons and full rows are kept whenever they fit.
- **B&W displays**: warnings and gauge tracks now render in white instead of invisible black.

## [0.2.0] - 2026-09-28

### Added

- **Configurable metrics**: choose what each panel displays from the settings.
- **MEDICAL**: up to 8 health metrics (BPM, steps, sleep, calories, distance, active minutes, resting calories, deep sleep).
- **ENVIRON**: toggle weather, wind, humidity, UV and sunrise/sunset individually.
- **SYSTEMS**: toggle battery and communication indicators.
- **Date display**: `DOW DD MON` shown under the time.
- **Hero mode**: larger time when the panel is alone on screen.

### Changed

- **SYSTEMS redesign**: battery bar and Bluetooth status on one line.

### Fixed

- **Constructor logos**: no more overlap with the time.
- **Bluetooth icon color**: follows the label color setting.
- **Android settings**: multi-select options now apply correctly on real phones.

## [0.1.0] - 2026-09-23

Initial release.

### Added

- **mobiGlas layout**: time, weather, health and battery panels in a clean sci-fi look.
- **Time formats**: 12h and 24h.
- **Live weather**: temperature, conditions, wind, humidity, UV index, sunrise/sunset.
- **Health**: heart rate and daily steps.
- **Logos**: 9 manufacturer logos to choose from.
- **Colors**: 5 accent colors with a reset button.
- **Panels**: toggle MEDICAL, ENVIRON and SYSTEMS on or off.

### Notes

- **B&W watches**: colors render as white and the logo is hidden, the screen is too narrow for both.
