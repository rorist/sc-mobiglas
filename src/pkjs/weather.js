// SC mobiGlas — weather via Open-Meteo (no API key)
// GPS position + current weather → AppMessage:
//   0 KEY_TEMP (Int8, rounded Celsius), 1 KEY_WEATHER (CString label),
//  10 KEY_WIND_SPEED (Int16 km/h),     11 KEY_WIND_DIR (Int16 degrees),
//  12 KEY_HUMIDITY (Int8 %),           13 KEY_UV (Int8 index),
//  14 KEY_SUNRISE (CString "HH:MM"),   15 KEY_SUNSET (CString "HH:MM")

// Cache: fresh (<10 min) results are kept in phone-side localStorage; reopening
// the watchface re-sends the cache instantly (no GPS / Open-Meteo round-trip).

// WMO weathercode → display label
function wmo_label(code) {
  if (code === 0) return 'CLEAR';
  if (code >= 1 && code <= 3) return 'CLOUDY';
  if (code === 45 || code === 48) return 'FOG';
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return 'RAIN';
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return 'SNOW';
  if (code >= 95 && code <= 99) return 'STORM';
  return 'N/A';
}

// Phone-side weather cache: a fresh (< TTL) payload is re-sent as-is when the
// watchface is reopened, skipping geolocation and the Open-Meteo request.
// localStorage only stores strings — the payload is JSON-serialized with its date.
var CACHE_KEY = 'sc-weather-cache';
var CACHE_TTL_MS = 10 * 60 * 1000; // 10 min, matches geolocation maximumAge

function load_cache() {
  try {
    var raw = localStorage.getItem(CACHE_KEY);
    if (!raw) return null;
    var c = JSON.parse(raw);
    if (!c || !c.p) return null;
    var age = Date.now() - c.t;
    if (!isFinite(age) || age < 0 || age > CACHE_TTL_MS) return null; // stale
    return { payload: c.p, age_s: Math.round(age / 1000) };
  } catch (e) {
    try { localStorage.removeItem(CACHE_KEY); } catch (e2) {} // corrupt: clean up
    return null;
  }
}

function save_cache(payload) {
  try {
    localStorage.setItem(CACHE_KEY, JSON.stringify({ t: Date.now(), p: payload }));
  } catch (e) {
    console.log('Weather cache save failed: ' + e.message);
  }
}

function send_weather(payload) {
  Pebble.sendAppMessage(payload, function () {
    console.log('Weather sent: ' + JSON.stringify(payload));
  }, function (err) {
    console.log('Weather send failed: ' + err);
  });
}

function int_or_skip(v) {
  return (typeof v === 'number' && isFinite(v)) ? Math.round(v) : null;
}

function hhmm(iso) {
  if (typeof iso !== 'string' || iso.indexOf('T') < 0) return null;
  return iso.slice(-5); // "2026-09-21T06:42" → "06:42"
}

function fetch_weather(lat, lon) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat.toFixed(4) +
    '&longitude=' + lon.toFixed(4) +
    '&current=temperature_2m,relative_humidity_2m,weather_code,' +
      'wind_speed_10m,wind_direction_10m,uv_index' +
    '&daily=sunrise,sunset' +
    '&timezone=auto';
  var req = new XMLHttpRequest();
  req.onload = function () {
    if (req.status !== 200) {
      console.log('Open-Meteo HTTP ' + req.status);
      return;
    }
    try {
      var data = JSON.parse(req.responseText);
      var cur = data.current;
      if (!cur || typeof cur.temperature_2m !== 'number') {
        console.log('Open-Meteo response missing current block');
        return;
      }
      var daily = data.daily || {};

      var keys = require('message_keys');
      var payload = {};
      payload[keys.KEY_TEMP] = Math.round(cur.temperature_2m);
      payload[keys.KEY_WEATHER] = wmo_label(cur.weather_code);
      if (typeof cur.wind_speed_10m === 'number') {
        payload[keys.KEY_WIND_SPEED] = Math.round(cur.wind_speed_10m);
      }
      if (typeof cur.wind_direction_10m === 'number') {
        payload[keys.KEY_WIND_DIR] = Math.round(cur.wind_direction_10m);
      }
      if (typeof cur.relative_humidity_2m === 'number') {
        payload[keys.KEY_HUMIDITY] = Math.round(cur.relative_humidity_2m);
      }
      if (typeof cur.uv_index === 'number') {
        payload[keys.KEY_UV] = Math.round(cur.uv_index);
      }
      if (daily.sunrise && daily.sunrise[0]) {
        payload[keys.KEY_SUNRISE] = hhmm(daily.sunrise[0]);
      }
      if (daily.sunset && daily.sunset[0]) {
        payload[keys.KEY_SUNSET] = hhmm(daily.sunset[0]);
      }
      send_weather(payload);
      save_cache(payload);
    } catch (e) {
      console.log('Open-Meteo parse error: ' + e.message);
    }
  };
  req.onerror = function () {
    console.log('Open-Meteo request error');
  };
  req.open('GET', url);
  req.send();
}

function get_position() {
  navigator.geolocation.getCurrentPosition(
    function (pos) {
      fetch_weather(pos.coords.latitude, pos.coords.longitude);
    },
    function (err) {
      console.log('Geolocation failed: ' + err.message);
    },
    { timeout: 10000, maximumAge: 600000 }
  );
}

function fetch() {
  var cached = load_cache();
  if (cached) {
    console.log('Weather cache hit (' + cached.age_s + 's old)');
    send_weather(cached.payload);
    return;
  }
  if (typeof navigator === 'undefined' || !navigator.geolocation) {
    console.log('Geolocation unavailable');
    return;
  }
  get_position();
}

module.exports = {
  fetch: fetch
};