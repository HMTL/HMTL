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

/* VERSION specific values ***************************************************/
#define HMTL_VERSION 1

#if HMTL_VERSION == 1
#define RED_LED    9
#define GREEN_LED 10
#define BLUE_LED  11

#define DEBUG_LED 13

#define DATA_PIN 12
#define CLOCK_PIN 8
#elif HMTL_VERSION == 2
#define RED_LED   10
#define GREEN_LED 11
#define BLUE_LED  13

#define DATA_PIN 12
#define CLOCK_PIN 8
#endif

/* Board specific values */
#define DEVICE_ID  2

#define NUM_PIXELS 105

/* XXX: Set to true to force the config to be written */
boolean force_write = false;
boolean wrote_config = false;

#define MAX_OUTPUTS 4
config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];
config_hdr_t readconfig;
config_max_t readoutputs[MAX_OUTPUTS];

config_rgb_t rgb_output;
config_pixels_t pixel_output;
config_value_t value_output;

int configOffset = -1;
PixelUtil pixels;


void config_init() {
  int out = 0;

  rgb_output.hdr.type = HMTL_OUTPUT_RGB;
  rgb_output.hdr.output = out;
  rgb_output.pins[0] = RED_LED;
  rgb_output.pins[1] = GREEN_LED;
  rgb_output.pins[2] = BLUE_LED;
  rgb_output.values[0] = 0;
  rgb_output.values[1] = 0;
  rgb_output.values[2] = 0;
  outputs[out] = &rgb_output.hdr;   out++;

  pixel_output.hdr.type = HMTL_OUTPUT_PIXELS;
  pixel_output.hdr.output = out;
  pixel_output.dataPin = DATA_PIN;
  pixel_output.clockPin = CLOCK_PIN;
  pixel_output.numPixels = NUM_PIXELS;
  pixel_output.type = WS2801_RGB;
  outputs[out] = &pixel_output.hdr; out++;

#ifdef DEBUG_LED
  value_output.hdr.type = HMTL_OUTPUT_VALUE;
  value_output.hdr.output = out;
  value_output.pin = DEBUG_LED;
  value_output.value = 0;
  outputs[out] = &value_output.hdr;   out++;
#endif

  hmtl_default_config(&config);
  config.address = DEVICE_ID;
  config.num_outputs = out;
  config.flags = 0;

  if (out > MAX_OUTPUTS) {
    DEBUG_ERR("Exceeded maximum outputs");
    DEBUG_ERR_STATE(DEBUG_ERR_INVALID);
  }
}

void setup() {
  Serial.begin(9600);

  readconfig.address = -1;
  configOffset = hmtl_read_config(&readconfig, 
				  readoutputs, 
				  MAX_OUTPUTS);
  if ((configOffset < 0) ||
      force_write) {
    // Setup and write the configuration
    config_init();

    configOffset = hmtl_write_config(&config, outputs);
    if (configOffset < 0) {
      DEBUG_ERR("Failed to write config");
    } else {
      wrote_config = true;
      DEBUG_PRINTLN(DEBUG_HIGH, "Wrote configuration to EEPROM");
    }

  } else {
    DEBUG_VALUELN(DEBUG_LOW, "Read config.  offset=", configOffset);
    memcpy(&config, &readconfig, sizeof (config_hdr_t));
    for (int i = 0; i < config.num_outputs; i++) {
      if (i >= MAX_OUTPUTS) {
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
    }
    case HMTL_OUTPUT_VALUE: {
      memcpy(&value_output, outputs[i], sizeof (value_output));
    }
    }
    hmtl_setup_output((output_hdr_t *)outputs[i], data);
  }

#ifdef DEBUG_LED
  pinMode(DEBUG_LED, OUTPUT);
#endif

  for (unsigned int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelRGB(i, 0, 0, 0);
  }
  pixels.update();
}

void loop() {

  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 255, 255, 255);  
  pixels.update();

#ifdef DEBUG_LED
  digitalWrite(value_output.pin, HIGH);
#endif

  delay(1000);
#ifdef DEBUG_LED
  digitalWrite(value_output.pin, LOW);
#endif

  digitalWrite(rgb_output.pins[0], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 255, 0, 0);  
  pixels.update();

  delay(1000);

  digitalWrite(rgb_output.pins[0], LOW);
  digitalWrite(rgb_output.pins[1], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 0, 255, 0);  
  pixels.update();

  delay(1000);

  digitalWrite(rgb_output.pins[1], LOW);
  digitalWrite(rgb_output.pins[2], HIGH);
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, 0, 0, 255);  
  pixels.update();

  delay(1000);

  digitalWrite(rgb_output.pins[2], LOW);
}
