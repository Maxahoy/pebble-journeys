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
        "type": "select",
        "messageKey": "ProgressUnit",
        "label": "Distance Units",
        "defaultValue": 0,
        "options": [
          { "label": "Miles", "value": 0 },
          { "label": "Kilometers", "value": 1 }
        ]
      }
    ]
  },
  {
    "type": "section",
    "items": [
      { "type": "heading", "defaultValue": "Appearance" },
      {
        "type": "toggle",
        "messageKey": "ShowBattery",
        "label": "Show Battery Bar",
        "description": "Shows a green/yellow/red charge bar at the top. When off, a thin themed stripe is shown instead.",
        "defaultValue": true
      },
      {
        "type": "toggle",
        "messageKey": "AutoTheme",
        "label": "Auto Theme (Experimental)",
        "description": "Uses your GPS location and current weather to pick a matching theme automatically. Overrides the manual Theme setting below.",
        "defaultValue": false
      },
      {
        "type": "select",
        "messageKey": "Theme",
        "label": "Theme",
        "defaultValue": 0,
        "options": [
          { "label": "Vaporwave", "value": 0 },
          { "label": "Beach", "value": 1 },
          { "label": "Mountain", "value": 2 },
          { "label": "Winter", "value": 3 },
          { "label": "City", "value": 4 },
          { "label": "Plains", "value": 5 },
          { "label": "Desert", "value": 6 }
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
      { "type": "heading", "defaultValue": "Debug / Overrides" },
      {
        "type": "text",
        "defaultValue": "GPS auto-detects day and distance when you shake your wrist. Use the override only if auto-detection isn't working."
      },
      {
        "type": "input",
        "messageKey": "CurrentDistance",
        "label": "Distance Override",
        "description": "Set to 0 (or leave blank) to use GPS auto-detection",
        "defaultValue": "0",
        "attributes": { "type": "number", "min": "0" }
      },
      {
        "type": "input",
        "messageKey": "TripDay",
        "label": "Trip Day Override",
        "description": "Set to 0 (or leave blank) to use GPS auto-detection. 1 = Day 1, 2 = Day 2, etc.",
        "defaultValue": "0",
        "attributes": { "type": "number", "min": "0", "max": "6" }
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
