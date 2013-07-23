/*
 * Write out an HTML config if it doesn't already exist
 */
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>


#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"


boolean wrote_config = false;

#define PIN_DEBUG_LED 13

#define MAX_OUTPUTS 3

config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];

config_value_t val_output;
config_rgb_t rgb_output;

void config_init() 
{
  val_output.hdr.type = HMTL_OUTPUT_VALUE;
  val_output.hdr.output = 0;
  val_output.pin = 9;
  val_output.value = 0;

  rgb_output.hdr.type = HMTL_OUTPUT_RGB;
  rgb_output.hdr.output = 1;
  rgb_output.pins[0] = 10;
  rgb_output.pins[1] = 11;
  rgb_output.pins[2] = 12;
  rgb_output.value[0] = 0;
  rgb_output.value[1] = 0;
  rgb_output.value[2] = 0;

  config.magic = HMTL_CONFIG_MAGIC;
  config.address = 1;
  config.num_outputs = 2;
  
  outputs[0] = &rgb_output.hdr;
  outputs[1] = &val_output.hdr;
}


void setup() 
{
  config_init();
  
  Serial.begin(9600);

  config_hdr_t readconfig;
  config_max_t readoutputs[MAX_OUTPUTS];
  if ((hmtl_read_config(&readconfig, readoutputs, MAX_OUTPUTS) < 0) ||
      (readconfig.address != config.address)) {
    if (hmtl_write_config(&config, outputs) < 0) {
      DEBUG_PRINTLN(0, "Failed to write config");
    } else {
      wrote_config = true;
    }
  }

  pinMode(PIN_DEBUG_LED, OUTPUT);
}


void loop() 
{
  static long last_update = 0;
  if (millis() - last_update > 1000) {
    DEBUG_VALUE(0, "My address:", config.address);
    DEBUG_VALUELN(0, " Was address written: ", wrote_config);
    last_update = millis();
  }

  blink_value(PIN_DEBUG_LED, config.address, 250, 4);
  delay(10);
}
