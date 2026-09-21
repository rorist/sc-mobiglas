// Clay configuration — SC mobiGlas settings page
// Types: https://github.com/pebble-dev/clay
module.exports = [
  {
    "type": "heading",
    "defaultValue": "SC mobiGlas"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Time"
      },
      {
        "type": "toggle",
        "messageKey": "KEY_12H",
        "label": "12-hour format",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Weather"
      },
      {
        "type": "toggle",
        "messageKey": "KEY_FAHRENHEIT",
        "label": "Fahrenheit",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Panels"
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_MEDICAL",
        "label": "Medical panel",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_ENVIRON",
        "label": "Environ panel",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_SYSTEMS",
        "label": "Systems panel",
        "defaultValue": true
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];