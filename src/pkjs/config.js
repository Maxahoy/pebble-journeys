module.exports = [
  {
    "type": "heading",
    "defaultValue": "Road Trip Tracker"
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Trip Info" },
      {
        "type": "input",
        "messageKey": "TripName",
        "label": "Trip Name",
        "defaultValue": "Summer Road Trip"
      },
      {
        "type": "input",
        "messageKey": "StartName",
        "label": "Start City",
        "description": "City name used to auto-calculate distances (e.g. Cincinnati, OH)",
        "defaultValue": ""
      },
      {
        "type": "input",
        "messageKey": "DestName",
        "label": "Destination City",
        "description": "Also used for destination weather",
        "defaultValue": ""
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Overnight Stops (in order)" },
      { "type": "input", "messageKey": "Waypoint1", "label": "Stop 1", "defaultValue": "" },
      { "type": "input", "messageKey": "Waypoint2", "label": "Stop 2", "defaultValue": "" },
      { "type": "input", "messageKey": "Waypoint3", "label": "Stop 3", "defaultValue": "" },
      { "type": "input", "messageKey": "Waypoint4", "label": "Stop 4", "defaultValue": "" },
      { "type": "input", "messageKey": "Waypoint5", "label": "Stop 5", "defaultValue": "" }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Today's Progress" },
      {
        "type": "text",
        "defaultValue": "Day and distance are auto-detected from GPS when you shake your wrist. Use the override below only if needed."
      },
      {
        "type": "input",
        "messageKey": "CurrentDistance",
        "label": "Distance Override (optional)",
        "description": "Leave 0 to use GPS auto-detection",
        "defaultValue": "0",
        "attributes": { "type": "number", "min": "0" }
      },
      {
        "type": "select",
        "messageKey": "ProgressUnit",
        "label": "Distance Units",
        "defaultValue": 0,
        "options": [
          { "label": "Miles", "value": 0 },
          { "label": "Kilometers", "value": 1 },
          { "label": "Percentage", "value": 2 }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Weather" },
      {
        "type": "toggle",
        "messageKey": "ShowWeather",
        "label": "Show Weather",
        "defaultValue": true
      },
      {
        "type": "select",
        "messageKey": "TempUnit",
        "label": "Temperature Unit",
        "defaultValue": 0,
        "options": [
          { "label": "Celsius (°C)", "value": 0 },
          { "label": "Fahrenheit (°F)", "value": 1 }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Danger Zone" },
      {
        "type": "text",
        "defaultValue": "Toggle 'Full Reset' ON then tap Save to wipe all saved distances and settings from the watch. Fixes stuck 0% progress."
      },
      {
        "type": "toggle",
        "messageKey": "ResetData",
        "label": "Full Reset",
        "defaultValue": false
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
