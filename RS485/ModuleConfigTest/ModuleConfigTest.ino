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

  /* Initialize the outputs */
  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_setup_output((output_hdr_t *)outputs[i], &pixels);
  }
}

#define DELAY 10
void loop() {

  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_test_output(outputs[i], &pixels);
  }

  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_update_output(outputs[i], &pixels);
  }

  delay(DELAY);
}
