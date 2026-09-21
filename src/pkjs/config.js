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
      },
      {
        "type": "select",
        "messageKey": "KEY_LOGO",
        "label": "Constructor logo",
        "defaultValue": "6",
        "options": [
          { "value": "0", "label": "None" },
          { "value": "1", "label": "Aegis Dynamics" },
          { "value": "2", "label": "Anvil Aerospace" },
          { "value": "3", "label": "Crusader Industries" },
          { "value": "4", "label": "RSI" },
          { "value": "5", "label": "MISC" },
          { "value": "6", "label": "Star Citizen" }
        ]
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