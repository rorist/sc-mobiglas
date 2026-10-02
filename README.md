# SC mobiGlas - Star Citizen Watchface for Pebble

A fan-made watchface for Pebble inspired by the Star Citizen mobiGlas holographic interface. Works on color (PT2), round (Round 2), and B&W (Pebble 2 Duo) watches.

## Features

- **Time**: 24h/12h format, optional date, hero mode with constructor logo
- **Weather**: temperature (°C/°F), conditions, wind, humidity, UV index, sunrise/sunset via Open-Meteo
- **Health**: heart rate, steps, sleep, calories, distance, active time, rest calories, deep sleep
- **Systems**: battery level with charge bar, Bluetooth connection status
- **9 constructor logos**: Aegis, Anvil, Crusader, RSI, Drake, Origin, Star Citizen, Frontier Fighters, Headhunters
- **Customizable**: 5 accent colors, per-panel metric toggles

## Installation

Install from the [Rebble app store](https://apps.repebble.com/e0270f35ea0b418589fa0866) or sideload the .pbw.

## Development

Requires the [Rebble SDK](https://developer.repebble.com/) (see the install guide there).

    pebble clean && pebble build             # build all 3 platforms
    pebble install --emulator emery          # emery | flint | gabbro
    pebble logs                              # stream watch logs
    pebble screenshot --no-open --emulator emery shot.png

### debug.sh (capture harness)

    ./debug.sh --list                       # list the config presets
    ./debug.sh --install all                # install + capture all cases, 3 emulators
    ./debug.sh --emu flint med-only         # one case on one emulator
    ./debug.sh --reset                      # restore default config

Captures land in debug/, debug/flint/, debug/gabbro/ with an index.md gallery.

## Legal

This is an unofficial fan project created under the [Star Citizen Fan Kit and Fandom FAQ](https://support.robertsspaceindustries.com/hc/en-us/articles/360006895793-Star-Citizen-Fankit-and-Fandom-FAQ). Not affiliated with Cloud Imperium Games or Roberts Space Industries.

Uses assets from the [Star Citizen Fankit](https://robertsspaceindustries.com/fankit).

## License

MIT

## Author

Rorist
