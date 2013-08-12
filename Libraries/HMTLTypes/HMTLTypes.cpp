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
      case HMTL_OUTPUT_PIXELS:
        return sizeof (config_pixels_t);
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
  hdr->version = HMTL_CONFIG_VERSION;
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
  hdr->version = HMTL_CONFIG_VERSION;
  hdr->address = 0;
  hdr->num_outputs = 0;
  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_default_config: address="), hdr->address);
}

/* Print out details of a config */
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
  DEBUG_VALUE(0, "hmtl_print_config: mag: ", hdr->magic);
  DEBUG_VALUE(0, " version: ", hdr->version);
  DEBUG_VALUE(0, " address: ", hdr->address);
  DEBUG_VALUELN(0, " outputs: ", hdr->num_outputs);

  for (int i = 0; i < hdr->num_outputs; i++) {
    output_hdr_t *out1 = (output_hdr_t *)outputs[i];
    DEBUG_VALUE(0, "addr=", (int)out1);
    DEBUG_VALUE(0, " type=", out1->type);
    DEBUG_VALUE(0, " out=", out1->output);
    DEBUG_PRINT(0, " - ");
    switch (out1->type) {
        case HMTL_OUTPUT_VALUE: 
        {
          config_value_t *out2 = (config_value_t *)out1;
          DEBUG_VALUE(0, "value pin=", out2->pin);
          DEBUG_VALUELN(0, " val=", out2->value);
          break;
        }
        case HMTL_OUTPUT_RGB:
        {
          config_rgb_t *out2 = (config_rgb_t *)out1;
          DEBUG_VALUE(0, "rgb pin0=", out2->pins[0]);
          DEBUG_VALUE(0, " pin1=", out2->pins[1]);
          DEBUG_VALUE(0, " pin2=", out2->pins[2]);
          DEBUG_VALUE(0, " val0=", out2->values[0]);
          DEBUG_VALUE(0, " val1=", out2->values[1]);
          DEBUG_VALUELN(0, " val2=", out2->values[2]);
          break;
        }
        case HMTL_OUTPUT_PROGRAM:
        {
          config_program_t *out2 = (config_program_t *)out1;
          DEBUG_VALUELN(0, "program val=", out2->value);
          break;
        }
        case HMTL_OUTPUT_PIXELS:
        {
          config_pixels_t *out2 = (config_pixels_t *)out1;
          DEBUG_VALUE(0, "pixels clock=", out2->clockPin);
          DEBUG_VALUE(0, " data=", out2->dataPin);
          DEBUG_VALUELN(0, " num=", out2->numPixels);
          break;
        }
        default:
        {
          DEBUG_PRINTLN(0, "Unknown type");
          break;
        }        
    }
  }
}

