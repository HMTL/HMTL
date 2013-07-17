/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
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

  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_read_config: read address="), hdr->address);

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

/* Fill in a config with default values */
void hmtl_default_config(config_hdr_t *hdr)
{
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->address = 0;
  hdr->output_address = 0;
  hdr->num_outputs = 0;
  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_default_config: address="), hdr->address);
}
