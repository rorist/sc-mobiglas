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
});

// Watch-driven refresh: the watch sends KEY_REQUEST_WEATHER every 30 min
// (phone-side setInterval is unreliable — PKJS can be killed in background).
Pebble.addEventListener('appmessage', function (e) {
  if (e.payload && e.payload['KEY_REQUEST_WEATHER']) {
    console.log('Weather refresh requested by watch');
    Weather.fetch();
  }
});
