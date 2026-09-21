// Clay customFn — SC mobiGlas
// Wires the "Reset colors to defaults" button (pure UI, no messageKey):
// clicking it sets the 5 color pickers back to their defaults in the page;
// the user then hits Save to send them to the watch.
module.exports = function(minified) {
  var clayConfig = this;
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    var btn = clayConfig.getItemById('reset-colors-btn');
    if (!btn) return;
    btn.on('click', function() {
      clayConfig.getItemByMessageKey('KEY_COLOR_TIME').set('aaffff');
      clayConfig.getItemByMessageKey('KEY_COLOR_VALUE').set('ffffff');
      clayConfig.getItemByMessageKey('KEY_COLOR_LABEL').set('55aaff');
      clayConfig.getItemByMessageKey('KEY_COLOR_HEADER').set('00aaff');
      clayConfig.getItemByMessageKey('KEY_COLOR_WARN').set('ff8800');
    });
  });
};