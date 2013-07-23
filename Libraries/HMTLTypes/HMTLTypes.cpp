/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "EEPromUtils.h"
#include "HMTLTypes.h"

int hmtl_output_size(output_hdr_t *output) 
{
  switch (output->type) {
      case HMTL_OUTPUT_VALUE:
        return sizeof (config_value_t);
      case HMTL_OUTPUT_RGB:
        return sizeof (config_rgb_t);
      case HMTL_OUTPUT_PROGRAM:
        return sizeof (config_program_t);
      default:
        DEBUG_ERR(F("hmtl_output_size: bad output type"));
        return -1;    
  }
}


int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs) 
{
  int addr;

  addr = EEPROM_safe_read(HMTL_CONFIG_ADDR,
                          (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR(F("hmtl_read_config: error reading config from eeprom"));
    return -1;
  }

  if (hdr->magic != HMTL_CONFIG_MAGIC) {
    DEBUG_ERR(F("hmtl_read_config: read config with invalid magic"));
    return -2;
  }

  if ((hdr->num_outputs > 0) && (max_outputs != 0)) {
    /* Read in the outputs if any were indicated and a buffer was provided */
    if (max_outputs < hdr->num_outputs) {
      DEBUG_ERR(F("hmtl_read_config: not enough outputs"));
      return -3;
    }
    for (int i = 0; i < hdr->num_outputs; i++) {
      addr = EEPROM_safe_read(addr,
                              (uint8_t *)&outputs[i], sizeof (config_max_t));
      if (addr <= 0) {
        DEBUG_ERR(F("hmtl_read_config: error reading outputs"));
      }
    }
  }

  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_read_config: read address="), hdr->address);

  return hdr->address;
}

int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[]) 
{
  int addr;
  hdr->magic = HMTL_CONFIG_MAGIC;
  addr = EEPROM_safe_write(HMTL_CONFIG_ADDR,
                           (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR(F("hmtl_write_config: failed to write config to EEProm"));
    return 0;
  }

  for (int i = 0; i < hdr->num_outputs; i++) {
    output_hdr_t *output = outputs[i];
    addr = EEPROM_safe_write(addr, (uint8_t *)output,
                             hmtl_output_size(output));
    if (addr < 0) {
      DEBUG_ERR(F("hmtl_write_config: failed to write outputs to EEProm"));
      return 0;
    }
  }

  return 1;
}

/* Fill in a config with default values */
void hmtl_default_config(config_hdr_t *hdr)
{
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->address = 0;
  hdr->num_outputs = 0;
  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_default_config: address="), hdr->address);
}
