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
        "type": "toggle",
        "messageKey": "KEY_SHOW_DATE",
        "label": "Show date",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "KEY_LOGO",
        "label": "Constructor logo",
        "defaultValue": "7",
        "options": [
          { "value": "0", "label": "None" },
          { "value": "1", "label": "Aegis Dynamics" },
          { "value": "2", "label": "Anvil Aerospace" },
          { "value": "3", "label": "Crusader Industries" },
          { "value": "4", "label": "RSI" },
          { "value": "5", "label": "Drake Interplanetary" },
          { "value": "6", "label": "Origin Jumpworks" },
          { "value": "7", "label": "Star Citizen" },
          { "value": "8", "label": "Frontier Fighters" },
          { "value": "9", "label": "Headhunters" }
        ]
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
        "type": "checkboxgroup",
        "messageKey": "KEY_MED_METRICS",
        "label": "Medical metrics",
        "defaultValue": [true, true, false, false, false, false, false, false],
        "options": [
          "Heart rate", "Steps", "Sleep", "Calories",
          "Distance", "Active time", "Resting kcal", "Deep sleep"
        ]
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_ENVIRON",
        "label": "Environ panel",
        "defaultValue": true
      },
      {
        "type": "checkboxgroup",
        "messageKey": "KEY_ENV_METRICS",
        "label": "Environ metrics",
        "defaultValue": [true, true, true, true, true, true],
        "options": ["Weather", "Wind", "Humidity", "UV", "Sunrise", "Sunset"]
      },
      {
        "type": "toggle",
        "messageKey": "KEY_SHOW_SYSTEMS",
        "label": "Systems panel",
        "defaultValue": true
      },
      {
        "type": "checkboxgroup",
        "messageKey": "KEY_SYS_METRICS",
        "label": "Systems metrics",
        "defaultValue": [true, true],
        "options": ["Battery", "Bluetooth"]
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
        "defaultValue": "Colors"
      },
      {
        "type": "color",
        "messageKey": "KEY_COLOR_TIME",
        "label": "Time color",
        "defaultValue": "#aaffff"
      },
      {
        "type": "color",
        "messageKey": "KEY_COLOR_VALUE",
        "label": "Value color",
        "defaultValue": "#ffffff"
      },
      {
        "type": "color",
        "messageKey": "KEY_COLOR_LABEL",
        "label": "Label color",
        "defaultValue": "#55aaff"
      },
      {
        "type": "color",
        "messageKey": "KEY_COLOR_HEADER",
        "label": "Header color",
        "defaultValue": "#00aaff"
      },
      {
        "type": "color",
        "messageKey": "KEY_COLOR_WARN",
        "label": "Warning color",
        "defaultValue": "#ff8800"
      },
      {
        "type": "button",
        "id": "reset-colors-btn",
        "defaultValue": "Reset colors to defaults"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save"
  }
];