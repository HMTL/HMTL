/*
 * This code instantiates a slave module
 */

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
#include "RS485Utils.h"


#define PIN_RS485_1     2
#define PIN_RS485_2     7 // XXX: This changed from 3 on the old ones
#define PIN_RS485_3     4

#define PIN_DEBUG_LED  13

RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

// Pixel strand outputs
Adafruit_WS2801 pixels;

#define MAX_OUTPUTS 5
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

  /* Setup the RS485 connection */  
  rs485.setup();
}

boolean b = false;

boolean updated[MAX_OUTPUTS] = { false, false, false };

void loop() {
  /* Check for messages to this address */
  read_state();

#if 0
  /* Populate the outputs with test data */
  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_test_output(outputs[i], &pixels);
    updated[i] = true;
  }
#endif

  /* If state changed then updated all outputs */
  for (int i = 0; i < config.num_outputs; i++) {
    if (updated[i]) {
      output_hdr_t *out = outputs[i];
      hmtl_update_output(out, &pixels);
      updated[i] = false;
    }
  }

  blink_value(PIN_DEBUG_LED, config.address, 500, 4);
}

void read_state() 
{
  unsigned int msglen;

  const byte *data = rs485.getMsg(config.address, &msglen);

  if (data != NULL) {
    if (msglen < sizeof (output_hdr_t)) {
      DEBUG_ERR(F("ERROR: read_state: msglen less than minimum size"));
      return;
    }

    output_hdr_t *hdr = (output_hdr_t *)data;
    if (hdr->output >= config.num_outputs) {
      DEBUG_ERR(F("ERROR: read_state: too many outputs"));
      return;
    }

    output_hdr_t *out = outputs[hdr->output];
    if (hdr->type != out->type) {
      DEBUG_ERR(F("ERROR: read_state: wrong type for output"));
      return;
    }
    if (msglen < hmtl_msg_size(hdr)) {
      DEBUG_ERR(F("ERROR: read_state: msglen less than type's size"));
      return;
    }

    switch (hdr->type) {
        case HMTL_OUTPUT_VALUE: 
        {
          msg_value_t *val = (msg_value_t *)hdr;
          config_value_t *out2 = (config_value_t *)out;
          out2->value = val->value;
          break;
        }
        case HMTL_OUTPUT_RGB:
        {
          msg_rgb_t *rgb = (msg_rgb_t *)hdr;
          config_rgb_t *out2 = (config_rgb_t *)out;
          out2->values[0] = rgb->values[0];
          out2->values[1] = rgb->values[1];
          out2->values[2] = rgb->values[2];
          break;
        }
        case HMTL_OUTPUT_PROGRAM:
        {
          msg_program_t *prog = (msg_program_t *)hdr;
          config_program_t *out2 = (config_program_t *)out;
          for (int i = 0; i < MAX_PROGRAM_VAL; i++) {
            out2->values[i] = prog->values[i];
          }
          break;
        }
        case HMTL_OUTPUT_PIXELS:
        {
/*
  XXX - This should do something, probably a program of some sort
          config_pixels_t *out2 = (config_pixels_t *)out;
          static int currentPixel = 0;
          pixels.setPixelRGB(currentPixel, 0, 0, 0);
          currentPixel = (currentPixel + 1) % pixels.numPixels();
          pixels.setPixelRGB(currentPixel, 255, 0, 0);
          pixels.update();
          break;
*/
        }
    }
    Serial.println();

    updated[hdr->output] = true;
  }
}
