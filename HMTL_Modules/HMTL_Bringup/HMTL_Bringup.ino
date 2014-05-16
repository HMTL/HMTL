#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "Wire.h"
#include "MPR121.h"
#include "SerialCLI.h"
#include "RS485Utils.h"

/******/

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_hdr_t readconfig;
config_max_t readoutputs[HMTL_MAX_OUTPUTS];

config_rgb_t rgb_output;
config_pixels_t pixel_output;
config_value_t value_output;
config_rs485_t rs485_output;
boolean has_value = false;


int configOffset = -1;
PixelUtil pixels;

void setup() {
  Serial.begin(9600);

  // XXX Update this to hmtl_setup()
  readconfig.address = -1;
  configOffset = hmtl_read_config(&readconfig, 
				  readoutputs, 
				  HMTL_MAX_OUTPUTS);
  if (configOffset < 0) {
    DEBUG_ERR("Failed to read config");
    DEBUG_ERR_STATE(1);
  } else {
    DEBUG_VALUELN(DEBUG_LOW, "Read config.  offset=", configOffset);
    memcpy(&config, &readconfig, sizeof (config_hdr_t));
    for (int i = 0; i < config.num_outputs; i++) {
      if (i >= HMTL_MAX_OUTPUTS) {
        DEBUG_VALUELN(0, "Too many outputs:", config.num_outputs);
        return;
      }
      outputs[i] = (output_hdr_t *)&readoutputs[i];
    }
  }

  DEBUG_VALUE(DEBUG_HIGH, "Config size:", configOffset - HMTL_CONFIG_ADDR);
  DEBUG_VALUELN(DEBUG_HIGH, " end:", configOffset);
  DEBUG_COMMAND(DEBUG_HIGH, hmtl_print_config(&config, outputs));

  /* Initialize the outputs */
  for (int i = 0; i < config.num_outputs; i++) {
    void *data = NULL;
    switch (((output_hdr_t *)outputs[i])->type) {
    case HMTL_OUTPUT_PIXELS: data = &pixels; break;
    case HMTL_OUTPUT_RGB: {
      memcpy(&rgb_output, outputs[i], sizeof (rgb_output));
      break;
    }
    case HMTL_OUTPUT_VALUE: {
      memcpy(&value_output, outputs[i], sizeof (value_output));
      has_value = true;
      break;
    }
    }
    hmtl_setup_output((output_hdr_t *)outputs[i], data);
  }

  for (unsigned int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelRGB(i, 0, 0, 0);
  }
  pixels.update();
}

void loop() {

  DEBUG_PRINTLN(0, "White");
  if (has_value) digitalWrite(value_output.pin, HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 255, 255, 255);  
  pixels.update();

  delay(1000);

  DEBUG_PRINTLN(0, "Red");
  if (has_value) digitalWrite(value_output.pin, LOW);
  digitalWrite(rgb_output.pins[0], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 255, 0, 0);  
  pixels.update();

  delay(1000);

  DEBUG_PRINTLN(0, "Green");
  digitalWrite(rgb_output.pins[0], LOW);
  digitalWrite(rgb_output.pins[1], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 0, 255, 0);  
  pixels.update();

  delay(1000);

  DEBUG_PRINTLN(0, "Blue");
  digitalWrite(rgb_output.pins[1], LOW);
  digitalWrite(rgb_output.pins[2], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 0, 0, 255);  
  pixels.update();

  delay(1000);

  digitalWrite(rgb_output.pins[2], LOW);
}
