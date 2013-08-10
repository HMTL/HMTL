window.devices =
  [
    {
      "address": "12",
      "name": "Flame effects",
      "controls": [
        {
          "id": "0",
          "name": "Ignitor 1",
          "type": "switch",
          "default": "off",
          "description": "Enable the ignitor for the pilot light"
        },
        {
          "id": "1",
          "name": "Poofer 1",
          "type": "execute_with_integer_value",
          "min": 100,
          "max": 2000,
          "default" : 100,
          "description": "Trigger the poofer for <value> ms"
        }
      ]
    },
    {
      "address": "25",
      "name": "Tiki head",
      "controls": [
        {
          "id": "0",
          "name": "Head color",
          "type": "hsl_color_picker_rgb_output",
          "default": "25,25,25",
          "description": "Set the color of the tiki head"
        }
      ]
    }
  ];