// SC mobiGlas — PebbleKit JS entry point
// Clay handles showConfiguration / webviewclosed automatically.
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

Pebble.addEventListener('ready', function () {
  console.log('SC mobiGlas PKJS ready');
  // TODO: weather fetch (weather.js) + calendar send
});