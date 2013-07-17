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

config_hdr_t readconfig;
config_hdr_t config;

boolean wrote_config = false;

#define PIN_DEBUG_LED 13

void setup() 
{
  Serial.begin(9600);

  config.magic = HMTL_CONFIG_MAGIC;
  config.address = 0;
  config.output_address = 0;
  config.num_outputs = 0;

  if ((hmtl_read_config(&readconfig) < 0) ||
      (readconfig.address != config.address)) {
    if (hmtl_write_config(&config) < 0) {
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

  blink_value(PIN_DEBUG_LED, config.address, 500, 4);
  delay(10);
}
