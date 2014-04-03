/*******************************************************************************
 * Convert an older HMTL config to the current version
 *
 * Author: Adam Phelps
 * License: Creative Commons Attribution-Non-Commercial
 * Copyright: 2014
 ******************************************************************************/

#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "Wire.h"
#include "EEPROM.h"
#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "SerialCLI.h"
#include "MPR121.h"
#include "RS485Utils.h"
#include "PixelUtil.h"

#define DEBUG_PIN 13

config_hdr_t config;

SerialCLI serialcli(128, cliHandler);

void setup()
{
  Serial.begin(9600);

  pinMode(DEBUG_PIN, OUTPUT);

  /* Read the input version */
  config_hdr_t readconfig;
  if (hmtl_read_config(&readconfig, NULL, 0) < 0) {
    DEBUG_ERR("Failed to read config, nothing to convert");
    DEBUG_ERR_STATE(1);
  }

  DEBUG_PRINTLN(DEBUG_LOW, "Current config:");
  hmtl_print_config(&readconfig, NULL);
}

void loop()
{
  serialcli.checkSerial();

  blink_value(DEBUG_PIN, config.address, 250, 4);
  delay(10);
}

uint8_t hardware_version = 0;
uint16_t address = 0;

/*
 * a <address> - Set device address
 * h <version> - Set hardware version
 * write - Write out the configuration
 */
void cliHandler(char **tokens, byte numtokens) {
  
  switch (tokens[0][0]) {
  case 'a': {
    if (numtokens < 2) return;
   address = atoi(tokens[1]);
    DEBUG_VALUELN(DEBUG_LOW, "Set device address:", address);
    break;
  }
  case 'h': {
    if (numtokens < 2) return;
    hardware_version = atoi(tokens[1]);
    DEBUG_VALUELN(DEBUG_LOW, "Set hardware version:", hardware_version);
    break;
  }

  case 'w': {
    if (strcmp(tokens[0], "write") == 0) {
      write_config();
    }
    break;
  }

  }
}

/*
 * Update the config, switching versions if necessary
 */
void write_config() {
  config_hdr_t readconfig;
  if (hmtl_read_config(&readconfig, NULL, 0) < 0) {
    DEBUG_ERR("Failed to read config, nothing to convert");
    DEBUG_ERR_STATE(1);
  }

  DEBUG_PRINTLN(DEBUG_LOW, "Current config:");
  hmtl_print_config(&readconfig, NULL);

  uint8_t old_version = readconfig.protocol_version;
  uint8_t new_version = HMTL_CONFIG_VERSION;

  /* Setup the config to be written */
  config.magic = HMTL_CONFIG_MAGIC;
  config.protocol_version = new_version;
  config.reserved = 0;

  if ((old_version == 1) && (new_version == 2)) {
    config_hdr_v1_t *old_hdr = (config_hdr_v1_t *)&readconfig;
    /* Shift all data besides the config */
    EEPROM_shift(HMTL_CONFIG_ADDR + sizeof (config_hdr_v1_t),
		 sizeof (config_hdr_v2_t) - sizeof (config_hdr_v1_t));

    /* Create the config in the new version and write it out*/
    if (address != 0) {
      config.address = address;
    } else {
      config.address = old_hdr->address;
    }

    config.hardware_version = hardware_version;
    config.num_outputs = old_hdr->num_outputs;
    config.flags = old_hdr->flags;
  } else if ((new_version == 2) && (new_version == old_version)) {
    config_hdr_v2_t *old_hdr = (config_hdr_v2_t *)&readconfig;
    if (address != 0) {
      config.address = address;
    } else {
      config.address = old_hdr->address;
    }

    if (hardware_version != 0) {
      config.hardware_version = hardware_version;
    } else {
      config.hardware_version = old_hdr->hardware_version;
    }

    config.num_outputs = old_hdr->num_outputs;
    config.flags = old_hdr->flags;
  } else {
    DEBUG_ERR("Invalid protocol versions");
    DEBUG_ERR_STATE(1);
  }
  
  if (hmtl_write_config(&config, NULL) < 0) {
    DEBUG_ERR("Failed to write new config");
    DEBUG_ERR_STATE(2);
  }

  hmtl_print_config(&config, NULL);

}

