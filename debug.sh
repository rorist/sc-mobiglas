#!/bin/bash
# debug.sh — fast debug harness for sc-mobiglas (no recompile needed)
# Applies a config preset in ONE send-app-message invocation (multi-key),
# captures a named screenshot into debug/, restores defaults.
#
# Usage:
#   ./debug.sh [--install] <case>   run one case on ALL 3 platforms (emery/flint/gabbro)
#   ./debug.sh [--install] --all    iterate all cases on each platform (+ per-platform index.md)
#   ./debug.sh --emu <emu> ...      restrict to ONE emulator (emery, flint, gabbro)
#   ./debug.sh --list               list available cases
#   ./debug.sh --reset              send default config on each platform and exit
#
# Without --emu the action runs on emery, flint AND gabbro (captures land in
# debug/, debug/flint/, debug/gabbro/). --install installs the current build
# on each platform first and seeds color defaults, sample weather and debug
# health values so captures are usable right away.

EMU=
INSTALL=0
while [ $# -gt 0 ]; do
  case "$1" in
    --emu)      EMU=$2; shift 2 ;;
    --install) INSTALL=1; shift ;;
    *) break ;;
  esac
done

# --- message keys (list-format numbering, see package.json) ---
K_MED=10000        # 8 items: BPM STEPS SLEEP KCAL DIST ACT RKCAL DSLEEP
K_ENV=10008        # 6 items: WEATHER WIND HUM UV SUNRISE SUNSET
K_SYS=10014       # 2 items: BAT COM
K_SHOW_DATE=10019
K_12H=10020
K_FAHRENHEIT=10021
K_SHOW_MED=10022
K_SHOW_ENV=10023
K_SHOW_SYS=10024
K_LOGO=10031       # CString
K_COL_TIME=10032   # 0xAAFFFF
K_COL_VALUE=10033  # 0xFFFFFF
K_COL_LABEL=10034  # 0x55AAFF
K_COL_HEADER=10035 # 0x00AAFF
K_COL_WARN=10036   # 0xFF8800
K_TEMP=10016       # Int8 — must be sent with K_WEATHER in the SAME message
K_WEATHER=10017    # CString
K_WIND_SPEED=10025
K_WIND_DIR=10026
K_HUMIDITY=10027
K_UV=10028
K_SUNRISE=10029    # CString "HH:MM"
K_SUNSET=10030     # CString "HH:MM"
K_DBG_BPM=10037     # debug health overrides (KEY_DEBUG_*)
K_DBG_STEPS=10038
K_DBG_SLEEP=10039   # seconds
K_DBG_KCAL=10040
K_DBG_DISTANCE=10041  # meters
K_DBG_ACTIVE=10042  # seconds
K_DBG_RKCAL=10043
K_DBG_DSLEEP=10044  # seconds

INT_ARGS=()

mask_args() { # base n mask
  local base=$1 n=$2 mask=$3
  for ((i = 0; i < n; i++)); do
    INT_ARGS+=( $((base + i))=$(( (mask >> i) & 1 )) )
  done
}

# One invocation: metric masks + display flags + logo (colors via send_color_defaults)
send_config() { # med_mask env_mask sys_mask 12h F date panels_med panels_env panels_sys logo
  INT_ARGS=()
  mask_args $K_MED 8 $1
  mask_args $K_ENV 6 $2
  mask_args $K_SYS 2 $3
  INT_ARGS+=( $K_SHOW_DATE=$6 $K_12H=$4 $K_FAHRENHEIT=$5
              $K_SHOW_MED=$7 $K_SHOW_ENV=$8 $K_SHOW_SYS=$9 )
  pebble send-app-message --emulator "$EMU" --int "${INT_ARGS[@]}" \
    --string $K_LOGO=${10} >/dev/null
}

send_defaults() { send_config 3 63 3 0 0 1 1 1 1 7; }

send_color_defaults() {
  pebble send-app-message --emulator "$EMU" --int \
    $K_COL_TIME=11206655 $K_COL_VALUE=16777215 $K_COL_LABEL=5614335 \
    $K_COL_HEADER=43775 $K_COL_WARN=16746496 >/dev/null
}

# Sample weather so ENVIRON captures show real data (temp + condition in the
# same AppMessage or appmessage.c ignores the pair)
send_weather() {
  pebble send-app-message --emulator "$EMU" \
    --int $K_TEMP=16 $K_WIND_SPEED=8 $K_WIND_DIR=135 $K_HUMIDITY=74 $K_UV=3 \
    --string $K_WEATHER=CLOUDY $K_SUNRISE=07:26 $K_SUNSET=19:14 >/dev/null
}

# Debug health overrides (appmessage.c dispatches KEY_DEBUG_*): values chosen
# to expose layout/color issues — varied ring fills incl. one near-empty.
send_debug_health() {
  pebble send-app-message --emulator "$EMU" --int \
    $K_DBG_BPM=72 $K_DBG_STEPS=8000 $K_DBG_SLEEP=21600 $K_DBG_KCAL=450 \
    $K_DBG_DISTANCE=3500 $K_DBG_ACTIVE=2700 $K_DBG_RKCAL=300 \
    $K_DBG_DSLEEP=3600 >/dev/null
}

# Clear the debug health overrides (val < 0 = clear, per medical_panel_set_debug)
send_debug_health_clear() {
  pebble send-app-message --emulator "$EMU" --int \
    $K_DBG_BPM=-1 $K_DBG_STEPS=-1 $K_DBG_SLEEP=-1 $K_DBG_KCAL=-1 \
    $K_DBG_DISTANCE=-1 $K_DBG_ACTIVE=-1 $K_DBG_RKCAL=-1 \
    $K_DBG_DSLEEP=-1 >/dev/null
}

shot() {
  pebble screenshot --no-open --emulator "$EMU" "$OUT/shot_$1.png" >/dev/null 2>&1 || return 1
  echo "captured $OUT/shot_$1.png"
}

run_case() {
  local med env sys h f d pm pe ps lg
  case "$1" in
    all)          med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    med-only)     med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=0; ps=0; lg=7 ;;
    env-only)     med=3;  env=63; sys=3; h=0; f=0; d=1; pm=0; pe=1; ps=0; lg=7 ;;
    sys-only)     med=3;  env=63; sys=3; h=0; f=0; d=1; pm=0; pe=0; ps=1; lg=7 ;;
    time-only)    med=3;  env=63; sys=3; h=0; f=0; d=1; pm=0; pe=0; ps=0; lg=7 ;;
    med+sys)      med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=0; ps=1; lg=7 ;;
    env+sys)      med=3;  env=63; sys=3; h=0; f=0; d=1; pm=0; pe=1; ps=1; lg=7 ;;
    med-1)        med=1;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    med-2)        med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    med-4)        med=15; env=63; sys=3; h=0; f=0; d=1; pm=1; pe=0; ps=0; lg=7 ;;
    med-8)        med=255; env=63; sys=3; h=0; f=0; d=1; pm=1; pe=0; ps=0; lg=7 ;;
    env-full)     med=3;  env=7;  sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    env-halves)   med=3;  env=60; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    env-lone)     med=3;  env=7;  sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    sys-bat-only) med=3;  env=63; sys=1; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    sys-com-only) med=3;  env=63; sys=2; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    sys-empty)    med=3;  env=63; sys=0; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    nodate)       med=3;  env=63; sys=3; h=0; f=0; d=0; pm=1; pe=1; ps=1; lg=7 ;;
    nologo)       med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=0 ;;
    12h)          med=3;  env=63; sys=3; h=1; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    fahrenheit)   med=3;  env=63; sys=3; h=0; f=1; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    logo-[0-9])   med=3;  env=63; sys=3; h=0; f=0; d=1; pm=1; pe=1; ps=1; lg=${1#logo-} ;;
    logo-nodate)  med=3;  env=63; sys=3; h=0; f=0; d=0; pm=1; pe=1; ps=1; lg=7 ;;
    logo-12h)     med=3;  env=63; sys=3; h=1; f=0; d=1; pm=1; pe=1; ps=1; lg=7 ;;
    *) echo "unknown case: $1"; exit 1 ;;
  esac
  send_config "$med" "$env" "$sys" "$h" "$f" "$d" "$pm" "$pe" "$ps" "$lg"
}

CASES=(all med-only env-only sys-only time-only med+sys env+sys med-1 med-2 med-4 med-8 env-full env-halves env-lone sys-bat-only sys-com-only sys-empty nodate nologo 12h fahrenheit logo-1 logo-2 logo-3 logo-4 logo-5 logo-6 logo-7 logo-8 logo-9 logo-nodate logo-12h)

if [ "$1" = "--list" ]; then
  printf '%s\n' "${CASES[@]}"
  exit 0
fi

if [ -z "$1" ]; then
  echo "usage: ./debug.sh [--emu emu] [--install] <case> | --all | --list | --reset"
  echo "default: runs on emery, flint and gabbro; --emu restricts to one"
  echo "cases: ./debug.sh --list"
  exit 1
fi

run_platform() { # $1 = emulator, $2 = action (case, --all or --reset)
  EMU=$1
  OUT=debug
  [ "$EMU" != "emery" ] && OUT="debug/$EMU"
  mkdir -p "$OUT"
  if [ "$INSTALL" = "1" ]; then
    pebble install --emulator "$EMU" || { echo "install failed on $EMU — is the emulator running?"; return 1; }
    send_defaults
    send_color_defaults
    send_weather
    send_debug_health
  fi
  if [ "$2" = "--reset" ]; then
    send_defaults
    send_color_defaults
    send_debug_health_clear
    echo "default config sent to $EMU"
    return 0
  fi
  if [ "$2" = "--all" ]; then
    {
      echo "# debug.sh captures ($EMU)"
      echo
      for c in "${CASES[@]}"; do
        run_case "$c"
        sleep 0.6
        shot "$c"
        send_defaults
      done
      echo
      for c in "${CASES[@]}"; do echo "## $c"; echo '![](shot_'"$c"'.png)'; echo; done
    } > "$OUT/index.md"
    echo "batch done on $EMU — $OUT/index.md"
    return 0
  fi
  run_case "$2"
  sleep 0.6
  shot "$2"
  send_defaults
}

if [ -z "$EMU" ]; then
  PLATFORMS=(emery flint gabbro)
else
  PLATFORMS=("$EMU")
fi

for p in "${PLATFORMS[@]}"; do
  run_platform "$p" "$1"
done
