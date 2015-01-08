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
#include "FastLED.h"

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

SerialCLI serialcli(128, cliHandler);

void setup()
{
  Serial.begin(9600);

  pinMode(DEBUG_PIN, OUTPUT);

  /* Read the input version */
  config_hdr_t readconfig;
  int version;
  if ((version = read_config_hdr((uint8_t *)&readconfig, sizeof (readconfig))) < 0) {
    DEBUG_ERR("Failed to read config, nothing to convert");
    DEBUG_ERR_STATE(1);
  }

  DEBUG_PRINTLN(DEBUG_LOW, "Current config:");
  hmtl_print_config(&readconfig, NULL);
}

void loop()
{
  serialcli.checkSerial();

  //  blink_value(DEBUG_PIN, config.address, 250, 4);
  delay(10);
}

uint8_t hardware_version = 0;
uint16_t address = 0;
uint16_t device_id = 0;
uint8_t baud = BAUD_TO_BYTE(9600);
/*
 * a <address> - Set device address
 * h <version> - Set hardware version
 * d <id>      - Set device ID
 * b <baud>    - Set baud rate
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
  case 'd': {
    if (numtokens < 2) return;
    device_id = atoi(tokens[1]);
    DEBUG_VALUELN(DEBUG_LOW, "Set device id:", device_id);
    break;
  }
  case 'h': {
    if (numtokens < 2) return;
    hardware_version = atoi(tokens[1]);
    DEBUG_VALUELN(DEBUG_LOW, "Set hardware version:", hardware_version);
    break;
  }
  case 'b': {
    if (numtokens < 2) return;
    uint16_t newBaud = atoi(tokens[1]);
    if ((newBaud < 1200) || (newBaud > 115200)) {
      DEBUG_VALUELN(DEBUG_ERROR, "Invalid baud:", newBaud);
      return;
    }
    baud = BAUD_TO_BYTE(newBaud);
    DEBUG_VALUELN(DEBUG_LOW, "Set baud:", BYTE_TO_BAUD(baud));
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
  uint8_t read_buffer[128];

  int old_version;
  if ((old_version = read_config_hdr(read_buffer, sizeof (read_buffer))) < 0) {
    DEBUG_ERR("Failed to read config, nothing to convert");
    DEBUG_ERR_STATE(1);
  }

  DEBUG_PRINTLN(DEBUG_LOW, "Current config:");
  hmtl_print_config((config_hdr_t *)read_buffer, NULL);

  config_hdr_v3_t config;
  int new_version = HMTL_CONFIG_VERSION;
  if ((old_version == 2) && (new_version == 3)) {
    config.magic = HMTL_CONFIG_MAGIC;
    config.protocol_version = new_version;

    config_hdr_v2_t *old_hdr = (config_hdr_v2_t *)read_buffer;
    /* Shift all data besides the config */
    EEPROM_shift(HMTL_CONFIG_ADDR + sizeof (config_hdr_v2_t),
		 sizeof (config_hdr_v3_t) - sizeof (config_hdr_v2_t));

    /* Create the config in the new version and write it out */
    if (address != 0) {
      config.address = address;
    } else {
      config.address = old_hdr->address;
    }

    if (device_id != 0) {
      config.device_id = device_id;
    } else {
      config.device_id = old_hdr->address;
    }

    if (hardware_version != 0) {
      config.hardware_version = hardware_version;
    } else {
      config.hardware_version = old_hdr->hardware_version;
    }

    config.baud = baud;
    
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

int read_config_hdr(uint8_t *buff, int buff_size) {
  int addr = EEPROM_safe_read(HMTL_CONFIG_ADDR,
			      buff, buff_size);
  if ((addr - HMTL_CONFIG_ADDR) < 2) {
    DEBUG_ERR("Header read was too small");
    return -1;
  }

  // First byte should be the magic value
  if (buff[0] != HMTL_CONFIG_MAGIC) {
    DEBUG_ERR("hmtl_read_config: read config with invalid magic");
    return -2;
  }

  DEBUG_VALUELN(0, "Read version ", buff[1]);

  // Second byte is the protocol version
  return buff[1];
}
