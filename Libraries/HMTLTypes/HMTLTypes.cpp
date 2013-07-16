/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

#include "Debug.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"


int hmtl_read_config(config_hdr_t *hdr) 
{
  if (EEPROM_safe_read(HMTL_CONFIG_ADDR,
                       (uint8_t *)hdr, sizeof (config_hdr_t)) < 0) {
    DEBUG_ERR(F("hmtl_read_config: error reading config from eeprom"));
    return -1;
  }

  if (hdr->magic != HMTL_CONFIG_MAGIC) {
    DEBUG_ERR(F("hmtl_read_config: read config with invalid magic"));
    return -2;
  }

  return hdr->address;
}

int hmtl_write_config(config_hdr_t *hdr) 
{
  hdr->magic = HMTL_CONFIG_MAGIC;
  if (EEPROM_safe_write(HMTL_CONFIG_ADDR,
                        (uint8_t *)hdr, sizeof (config_hdr_t)) < 0) {
    DEBUG_ERR(F("hmtl_write_config: failed to write config to EEProm"));
    return 0;
  }

  return 1;
}


