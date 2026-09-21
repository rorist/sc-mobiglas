// SC mobiGlas — PebbleKit JS entry point
// Clay handles showConfiguration / webviewclosed automatically.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var customClay = require('./custom-clay');
var clay = new Clay(clayConfig, customClay);
var Weather = require('./weather');

Pebble.addEventListener('ready', function () {
  console.log('SC mobiGlas PKJS ready');
  Weather.fetch();
  setInterval(Weather.fetch, Weather.FETCH_INTERVAL_MS);
});