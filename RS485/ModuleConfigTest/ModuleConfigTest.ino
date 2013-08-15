/*
 * This code is intended to verify that a module reads its config properly
 */

#include "EEPROM.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"
#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"

// PWM outputs
#define PWM_PINS 6
#define MAX_VAL 64
#define PWM_STEP 2

// Pixel strand outputs
PixelUtil pixels;

#define MAX_OUTPUTS 3
config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];
config_max_t readoutputs[MAX_OUTPUTS];

void setup() {
  Serial.begin(9600);
  
  /* Attempt to read the configuration */
  if (hmtl_read_config(&config, readoutputs, MAX_OUTPUTS) < 0) {
    hmtl_default_config(&config);
    DEBUG_PRINTLN(DEBUG_LOW, "Using default config");
  }
  if (config.num_outputs > MAX_OUTPUTS) {
    DEBUG_VALUELN(0, "Too many outputs:", config.num_outputs);
    config.num_outputs = MAX_OUTPUTS;
  }
  for (int i = 0; i < config.num_outputs; i++) {
    outputs[i] = (output_hdr_t *)&readoutputs[i];
  }

  DEBUG_COMMAND(DEBUG_HIGH, hmtl_print_config(&config, outputs));

  for (int i = 0; i < config.num_outputs; i++) {
    output_hdr_t *out1 = (output_hdr_t *)outputs[i];
    hmtl_setup_output((output_hdr_t *)outputs[i], &pixels);
  }
}

#define DELAY 10
void loop() {

  for (int i = 0; i < config.num_outputs; i++) {

    output_hdr_t *out1 = (output_hdr_t *)outputs[i];
    switch (out1->type) {
        case HMTL_OUTPUT_VALUE: 
        {
          config_value_t *out2 = (config_value_t *)out1;
          out2->value = (out2->value + PWM_STEP) % MAX_VAL;
          analogWrite(out2->pin, out2->value);
          break;
        }
        case HMTL_OUTPUT_RGB:
        {
          config_rgb_t *out2 = (config_rgb_t *)out1;
          for (int j = 0; j < 3; j++) {
            out2->values[j] = (out2->values[j] + PWM_STEP + j) % MAX_VAL;
            analogWrite(out2->pins[j], out2->values[j]);
          }
          break;
        }
        case HMTL_OUTPUT_PROGRAM:
        {
//          config_program_t *out2 = (config_program_t *)out1;
          break;
        }
        case HMTL_OUTPUT_PIXELS:
        {
//          config_pixels_t *out2 = (config_pixels_t *)out1;
          static int currentPixel = 0;
          pixels.setPixelRGB(currentPixel, 0, 0, 0);
          currentPixel = (currentPixel + 1) % pixels.numPixels();
          pixels.setPixelRGB(currentPixel, 255, 0, 0);
          pixels.update();
          break;
        }
    }
  }

  delay(DELAY);
}
