// SC mobiGlas — PebbleKit JS entry point
// Workaround: the Rebble Android runtime mis-serializes raw JS booleans in
// AppMessage (only the last item of a checkboxgroup array survives with its
// true value). Clay converts toggle scalars to integers but leaves checkbox
// array items as booleans — normalize every boolean to 1/0 before sending.
// pypkjs (emulator) handles booleans correctly, phones do not.
// [SEND] log kept TEMPORARILY for #13 validation. Remove once confirmed.
var _origSend = Pebble.sendAppMessage;
Pebble.sendAppMessage = function (payload, ok, nack) {
  var norm = {};
  Object.keys(payload).forEach(function (k) {
    var v = payload[k];
    norm[k] = (v === true) ? 1 : (v === false) ? 0 : v;
  });
  return _origSend.call(Pebble, norm, ok, nack);
};

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
