/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

//#define DEBUG_LEVEL DEBUG_HIGH // XXX: Just ERROR logging adds 1K to program size
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"

#ifdef USE_PIXELUTIL
#include "PixelUtil.h"
#warning USE_PIXELUTIL is enabled
#else
#warning USE_PIXELUTIL is disabled
#endif

#ifdef USE_RS485
#include "RS485Utils.h"
#warning USE_RS485 is enabled
#else
#warning USE_RS485 is disabled
#endif

#ifdef USE_MPR121
#include "MPR121.h"
#warning USE_MPR121 is enabled
#else
#warning USE_MPR121 is disabled
#endif

#ifdef USE_XBEE
#include "XBeeSocket.h"
#warning USE_XBEE is enabled
#else
#warning USE_XBEE is disabled
#endif

int hmtl_output_size(output_hdr_t *output) 
{
  switch (output->type) {
    case HMTL_OUTPUT_VALUE:
    return sizeof (config_value_t);
    case HMTL_OUTPUT_RGB:
    return sizeof (config_rgb_t);
    case HMTL_OUTPUT_PROGRAM:
    return sizeof (config_program_t);
#ifdef USE_PIXELUTIL
    case HMTL_OUTPUT_PIXELS:
    return sizeof (config_pixels_t);
#endif
#ifdef USE_MPR121
    case HMTL_OUTPUT_MPR121:
    return sizeof (config_mpr121_t);
#endif
#ifdef USE_RS485
    case HMTL_OUTPUT_RS485:
    return sizeof (config_rs485_t);
#endif
#ifdef USE_XBEE
    case HMTL_OUTPUT_XBEE:
    return sizeof (config_xbee_t);
#endif
    default:
    DEBUG_ERR("hmtl_output_size: bad output type");
    return -1;    
  }
}

/*
 * Read in the HMTL config, returning the EEProm address following
 * what was read.
 */
int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs) 
{
  int addr;

  /* Zero the outputs */
  for (int i = 0; i < max_outputs; i++) {
    outputs[i].hdr.type = HMTL_OUTPUT_NONE;
  }

  addr = EEPROM_safe_read(HMTL_CONFIG_ADDR,
                          (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR("hmtl_read_config: error reading config from eeprom");
    return -1;
  }

  if (hdr->magic != HMTL_CONFIG_MAGIC) {
    DEBUG_ERR("hmtl_read_config: read config with invalid magic");
    return -2;
  }

  if (hdr->protocol_version != HMTL_CONFIG_VERSION) {
    DEBUG1_VALUELN("hmtl_read_config: hdr has wrong protocol version:", hdr->protocol_version);
    return -3;
  }

  if ((hdr->num_outputs > 0) && (max_outputs != 0)) {
    /* Read in the outputs if any were indicated and a buffer was provided */
    if (max_outputs < hdr->num_outputs) {
      DEBUG_ERR("hmtl_read_config: not enough outputs");
      return -4;
    }
    for (int i = 0; i < hdr->num_outputs; i++) {
      addr = EEPROM_safe_read(addr,
                              (uint8_t *)&outputs[i], sizeof (config_max_t));
      if (addr <= 0) {
        DEBUG_ERR("hmtl_read_config: error reading outputs");
        return -5;
      }
    }
  }

  DEBUG2_VALUE("hmtl_read_config: size=", addr - HMTL_CONFIG_ADDR);
  DEBUG2_VALUE(" end=", addr);
  DEBUG2_VALUELN(" module address=", hdr->address);

  return addr;
}

/*
 * Write out the HMTL config, returning the EEProm address following
 * what was written.
 */
int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
  int addr;
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->protocol_version = HMTL_CONFIG_VERSION;
  addr = EEPROM_safe_write(HMTL_CONFIG_ADDR,
                           (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR("hmtl_write_config: failed to write config to EEProm");
    return -1;
  }

  if (outputs != NULL) {
    for (int i = 0; i < hdr->num_outputs; i++) {
      output_hdr_t *output = outputs[i];
      addr = EEPROM_safe_write(addr, (uint8_t *)output,
                               hmtl_output_size(output));
      if (addr < 0) {
        DEBUG_ERR("hmtl_write_config: failed to write outputs to EEProm");
        return -2;
      }
    }
  }

  DEBUG2_VALUE("hmtl_write_config: size=", addr - HMTL_CONFIG_ADDR);
  DEBUG2_VALUELN(" end=", addr);

  return addr;
}

/* Initialized the pins of an output */
int hmtl_setup_output(config_hdr_t *config, output_hdr_t *hdr, void *data)
{
  DEBUG4_VALUE("hmtl_setup_output: type=", hdr->type);
  switch (hdr->type) {
    case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        DEBUG4_PRINT(" value");
        pinMode(out->pin, OUTPUT);
        break;
      }
    case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        DEBUG4_PRINT(" rgb");
        for (int j = 0; j < 3; j++) {
          pinMode(out->pins[j], OUTPUT);
        }
        break;
      }
    case HMTL_OUTPUT_PROGRAM:
      {
        //        config_program_t *out = (config_program_t *)hdr;
        DEBUG4_PRINT(" program");
        break;
      }
#ifdef USE_PIXELUTIL
    case HMTL_OUTPUT_PIXELS:
      {
        DEBUG4_PRINT(" pixels");
        if (data != NULL) {
          config_pixels_t *out = (config_pixels_t *)hdr;
          PixelUtil *pixels = (PixelUtil *)data;
          pixels->init(out->numPixels,
                       out->dataPin,
                       out->clockPin,
                       out->type);
        } else {
          DEBUG_ERR("Expected PixelUtil data struct for pixel configs");
          return -1;
        }
        break;
      }
#endif
#ifdef USE_MPR121
    case HMTL_OUTPUT_MPR121:
      {
        DEBUG4_PRINTLN(" mpr121");
        if (data != NULL) {
          config_mpr121_t *out = (config_mpr121_t *)hdr;
          MPR121 *capSensor = (MPR121 *)data;
          capSensor->init(out->irqPin,
                          out->useInterrupt,
                          START_ADDRESS,  // XXX - Only single address
                          false,          // XXX - No touch times
                          false);         // XXX - No auto enable
          for (int i = 0; i < MAX_MPR121_PINS; i++) {
            byte touch = out->thresholds[i] & 0x0F;
            byte release = (out->thresholds[i] & 0xF0) >> 4;
            if (touch || release) {
              capSensor->setThreshold(i, touch, release);
            }
          }
        } else {
          DEBUG_ERR("Expected MPR121 data struct for mpr121 configs");
          return -1;
        }
        break;
      }
#endif
#ifdef USE_RS485
    case HMTL_OUTPUT_RS485:
      {
        DEBUG4_PRINT(" rs485");
        if (data != NULL) {
          config_rs485_t *out = (config_rs485_t *)hdr;
          RS485Socket *rs485 = (RS485Socket *)data;
          rs485->init(out->recvPin, out->xmitPin, out->enablePin,
                      config->address,
                      RS485_RECV_BUFFER, false); // Set to true to enable debugging
        } else {
          DEBUG_ERR("Expected RS485Socket data struct for RS485 configs");
          return -1;
        }
        break;
      }
#endif
#ifdef USE_XBEE
    case HMTL_OUTPUT_XBEE:
      {
        DEBUG4_PRINT(" xbee");
        if (data != NULL) {
          config_xbee_t *out = (config_xbee_t *)hdr;
          XBeeSocket *xbs = (XBeeSocket *)data;
          XBee *xbee = new XBee(); // TODO: Should this be allocated by the top level sketch?
          xbs->init(xbee,
                    config->address);
        } else {
          DEBUG_ERR("Expected XBeeSocket data struct for Xbee configs");
          return -1;
        }
        break;
      }
#endif
    default:
      {
        DEBUG1_VALUELN("Invalid type", hdr->type);
        return -1;
      }
  }

  DEBUG4_PRINTLN("");

  return 0;
}

/* Perform an update of an output */
int hmtl_update_output(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
    case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;

        // On a non-PWM pin this outputs HIGH if value >= 128
        analogWrite(out->pin, out->value);
        DEBUG5_VALUE("hmtl_update_output: val pin=", out->pin);
        DEBUG5_VALUELN(" val=", out->value);
        break;
      }
    case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        for (int j = 0; j < 3; j++) {
          analogWrite(out->pins[j], out->values[j]);
        }
        break;
      }
    case HMTL_OUTPUT_PROGRAM:
      {
        //          config_program_t *out = (config_program_t *)hdr;
        break;
      }
    case HMTL_OUTPUT_PIXELS:
      {
#ifdef USE_PIXELUTIL
        if (data) {
          PixelUtil *pixels = (PixelUtil *)data;
          pixels->update();
        }
#endif
        break;
      }
    case HMTL_OUTPUT_MPR121:
      {
#ifdef USE_MPR121
        // XXX - Should this be reading the inputs?
#endif
        break;
      }
    case HMTL_OUTPUT_RS485:
      {
#ifdef USE_RS485
        // XXX - Should this be checking for data?
#endif
        break;
      }
    case HMTL_OUTPUT_XBEE:
      {
#ifdef USE_XBEE
        // XXX - Should this be checking for data?
#endif
        break;
      }
    default: 
      {
        DEBUG_ERR("hmtl_update_output: unknown type");
        return -1;
      }
  }

  return 0;
}

/* Set the indicated output to a 3 byte value */
void hmtl_set_output_rgb(output_hdr_t *output, void *object, uint8_t value[3]) {
  switch (output->type) {
    case HMTL_OUTPUT_VALUE:{
      config_value_t *val = (config_value_t *)output;
      val->value = value[0];
      break;
    }
    case HMTL_OUTPUT_RGB:{
      config_rgb_t *rgb = (config_rgb_t *)output;
      rgb->values[0] = value[0];
      rgb->values[1] = value[1];
      rgb->values[2] = value[2];
      break;
    }
    case HMTL_OUTPUT_PIXELS: {
#ifdef USE_PIXELUTIL
      PixelUtil *pixels = (PixelUtil *)object;
      pixels->setAllRGB(value[0], value[1], value[2]);
#endif
      break;
    }
    default: {
      DEBUG_ERR("hmtl_set_output_rgb: invalid output type");
      break;
    }

  }
}


/******************************************************************************
 * Configuration validation
 */

boolean hmtl_validate_header(config_hdr_t *hdr) {
  if (hdr->magic != HMTL_CONFIG_MAGIC) return false;
  switch (hdr->protocol_version) {
    case 1: {
      config_hdr_v1_t *hdr_v1 = (config_hdr_v1_t *)hdr;
      if (hdr_v1->num_outputs > HMTL_MAX_OUTPUTS) return false;
      break;
    }
    case 2: {
      config_hdr_v2_t *hdr_v2 = (config_hdr_v2_t *)hdr;
      if (hdr_v2->num_outputs > HMTL_MAX_OUTPUTS) return false;
      break;
    }
    case 3: {
      config_hdr_v3_t *hdr_v3 = (config_hdr_v3_t *)hdr;
      if (hdr_v3->num_outputs > HMTL_MAX_OUTPUTS) return false;
      break;
    }

    default: return false;
  }

  return true;
}

boolean hmtl_validate_value(config_value_t *val) {
  if (val->pin > 13) return false;
  return true;
}

boolean hmtl_validate_rgb(config_rgb_t *rgb) {
  uint32_t pinmap = 0;
  uint32_t pinbit;

  for (int pin = 0; pin < 3; pin++) {
    if (rgb->pins[pin] > 13) return false;
    pinbit = (1 << rgb->pins[pin]);
    if (pinmap & pinbit) return false;
    pinmap |= pinbit;
  }
  return true;
}

boolean hmtl_validate_pixels(config_pixels_t *pixels) {
  if (pixels->clockPin > 13) return false;
  if (pixels->dataPin > 13) return false;
  if (pixels->clockPin == pixels->dataPin) return false;
  return true;
}

boolean hmtl_validate_mpr121(config_mpr121_t *mpr121) {
  if (mpr121->irqPin > 13) return false;
  return true;
}

boolean hmtl_validate_rs485(config_rs485_t *rs485) {
  if (rs485->recvPin > 13) return false;
  if (rs485->xmitPin > 13) return false;
  if (rs485->enablePin > 13) return false;
  if (rs485->recvPin == rs485->xmitPin) return false;
  if (rs485->recvPin == rs485->enablePin) return false;
  if (rs485->enablePin == rs485->xmitPin) return false;
  return true;
}

boolean hmtl_validate_xbee(config_xbee_t *xbee) {
  if (xbee->recvPin > 13) return false;
  if (xbee->xmitPin > 13) return false;
  return true;
}

boolean hmtl_validate_config(config_hdr_t *hdr, output_hdr_t *outputs[],
                             int num_outputs) {
  uint32_t pinmap = 0;
  uint32_t pinbit;

  if (!hmtl_validate_header(hdr)) goto VALIDATE_ERROR;
  if (hdr->num_outputs != num_outputs) {
    DEBUG_ERR("Number of outputs does not match");
    return false;
  }

  /* Verify that all pins are unique */
  for (int i = 0; i < num_outputs; i++) {
    output_hdr_t *out = outputs[i];
    switch (out->type) {
      case HMTL_OUTPUT_VALUE: {
        config_value_t *out2 = (config_value_t *)out;
        if (!hmtl_validate_value(out2)) goto VALIDATE_ERROR;
        pinbit = (1 << out2->pin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        break;
      }
      case HMTL_OUTPUT_RGB: {
        config_rgb_t *out2 = (config_rgb_t *)out;
        if (!hmtl_validate_rgb(out2)) goto VALIDATE_ERROR;
        for (int pin = 0; pin < 3; pin++) {
          pinbit = (1 << out2->pins[pin]);
          if (pinmap & pinbit) goto PIN_ERROR;
          pinmap |= pinbit;
        }
        break;
      }
      case HMTL_OUTPUT_PIXELS: {
        config_pixels_t *out2 = (config_pixels_t *)out;
        if (!hmtl_validate_pixels(out2)) goto VALIDATE_ERROR;
        pinbit = (1 << out2->clockPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        pinbit = (1 << out2->dataPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        break;
      }
      case HMTL_OUTPUT_MPR121: {
        config_mpr121_t *out2 = (config_mpr121_t *)out;
        if (!hmtl_validate_mpr121(out2)) goto VALIDATE_ERROR;
        pinbit = (1 << out2->irqPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        break;
      }
      case HMTL_OUTPUT_RS485: {
        config_rs485_t *out2 = (config_rs485_t *)out;
        if (!hmtl_validate_rs485(out2)) goto VALIDATE_ERROR;
        pinbit = (1 << out2->recvPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;

        pinbit = (1 << out2->xmitPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;

        pinbit = (1 << out2->enablePin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        break;
      }
      case HMTL_OUTPUT_XBEE: {
        config_xbee_t *out2 = (config_xbee_t *)out;
        if (!hmtl_validate_xbee(out2)) goto VALIDATE_ERROR;
        pinbit = (1 << out2->recvPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;

        pinbit = (1 << out2->xmitPin);
        if (pinmap & pinbit) goto PIN_ERROR;
        pinmap |= pinbit;
        break;
      }

      default: {
        DEBUG_ERR("Invalid output type");
        return false;
      }
    }
  }

  return true;

 PIN_ERROR:
  DEBUG_ERR("Pin used multiple times");
  return false;
 VALIDATE_ERROR:
  DEBUG_ERR("A config was invalid");
  return false;

}


/******************************************************************************
 * Debug printing of configuration
 */

void hmtl_print_header(config_hdr_t *hdr) {
#if DEBUG_LEVEL >= 3
  DEBUG3_VALUE("  header: mag: ", hdr->magic);
  DEBUG3_VALUE(" protocol_version: ", hdr->protocol_version);
  DEBUG3_VALUE(" hardware_version: ", hdr->hardware_version);
  DEBUG3_VALUE(" baud: ", BYTE_TO_BAUD(hdr->baud));
  DEBUG3_VALUE(" device_id: ", hdr->device_id);
  DEBUG3_VALUE(" address: ", hdr->address);
  DEBUG3_VALUE(" outputs: ", hdr->num_outputs);
  DEBUG3_VALUELN(" flags: ", hdr->flags);
#endif
}

void hmtl_print_output(output_hdr_t *out) {
#if DEBUG_LEVEL >= 3
  DEBUG3_VALUE("  output ", out->output);
  DEBUG3_VALUE(" offset=", (int)out);
  DEBUG3_VALUE(" type=", out->type);
  DEBUG3_PRINT(" - ");
  switch (out->type) {
    case HMTL_OUTPUT_VALUE:
      {
        config_value_t *out2 = (config_value_t *)out;
        DEBUG3_VALUE("value pin=", out2->pin);
        DEBUG3_VALUELN(" val=", out2->value);
        break;
      }
    case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out2 = (config_rgb_t *)out;
        DEBUG3_VALUE("rgb pin0=", out2->pins[0]);
        DEBUG3_VALUE(" pin1=", out2->pins[1]);
        DEBUG3_VALUE(" pin2=", out2->pins[2]);
        DEBUG3_VALUE(" val0=", out2->values[0]);
        DEBUG3_VALUE(" val1=", out2->values[1]);
        DEBUG3_VALUELN(" val2=", out2->values[2]);
        break;
      }
    case HMTL_OUTPUT_PROGRAM:
      {
        config_program_t *out2 = (config_program_t *)out;
        DEBUG3_PRINTLN("program");
        for (int i = 0; i < MAX_PROGRAM_VAL; i++) {
          DEBUG3_VALUELN(" val=", out2->values[i]);
        }
        break;
      }
    case HMTL_OUTPUT_PIXELS:
      {
        config_pixels_t *out2 = (config_pixels_t *)out;
        DEBUG3_VALUE("pixels clock=", out2->clockPin);
        DEBUG3_VALUE(" data=", out2->dataPin);
        DEBUG3_VALUE(" num=", out2->numPixels);
        DEBUG3_VALUELN(" type=", out2->type);
        break;
      }
    case HMTL_OUTPUT_MPR121:
      {
        config_mpr121_t *out2 = (config_mpr121_t *)out;
        DEBUG3_VALUE("mpr121 irq=", out2->irqPin);
        DEBUG3_VALUE(" useInt=", out2->useInterrupt);
        for (int i = 0; i < MAX_MPR121_PINS; i++) {
          byte touch = out2->thresholds[i] & 0x0F;
          byte release = (out2->thresholds[i] & 0xF0) >> 4;
          if (touch || release) {
            DEBUG3_VALUE(" thresh=", i);
            DEBUG3_VALUE(",", touch);
            DEBUG3_VALUE(",", release);
          }
        }
        DEBUG_PRINT_END();
        break;
      }
    case HMTL_OUTPUT_RS485:
      {
        config_rs485_t *out2 = (config_rs485_t *)out;
        DEBUG3_VALUE("rs485 recv=", out2->recvPin);
        DEBUG3_VALUE(" xmit=", out2->xmitPin);
        DEBUG3_VALUE(" enable=", out2->enablePin);
        DEBUG_PRINT_END();
        break;
      }
    case HMTL_OUTPUT_XBEE
      {
        config_xbee_t *out2 = (config_xbee_t *)out;
        DEBUG3_VALUE("xbee recv=", out2->recvPin);
        DEBUG3_VALUE(" xmit=", out2->xmitPin);
        DEBUG_PRINT_END();
        break;
      }
    default:
      {
        DEBUG3_PRINTLN("Unknown type");
        break;
      }
  }
#endif
}

/* Print out details of a config */
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
#if DEBUG_LEVEL >= 3
  DEBUG3_PRINTLN("hmtl_print_config:");
  hmtl_print_header(hdr);

  if (outputs == NULL) 
    return;

  for (int i = 0; i < hdr->num_outputs; i++) {
    output_hdr_t *out = (output_hdr_t *)outputs[i];
    if (out == NULL) {
      DEBUG3_VALUE("Output ", i);
      DEBUG3_PRINTLN(" is NULL");
      continue;
    }
    hmtl_print_output(out);
  }
#endif
}


/*
 * Read in the EEProm config and initialize ouputs
 */
int32_t hmtl_setup(config_hdr_t *config, 
                   config_max_t readoutputs[], 

                   output_hdr_t *outputs[], 
                   void *objects[],
                   byte num_outputs,
		   
                   void *rs485,
                   void *xbee,
                   void *pixels,
                   void *mpr121,
                   config_rgb_t *rgb_output,
                   config_value_t *value_output,
		   
                   int *configOffset
                   ) {
  int32_t outputs_found = 0;

  int offset = hmtl_read_config(config,
                                readoutputs, 
                                num_outputs);
  if (offset < 0) {
    DEBUG_ERR("Failed to read configuration");
    DEBUG_ERR_STATE(12);
  }
  if (configOffset) *configOffset = offset;

  DEBUG2_VALUELN("Read config.  offset=", offset);

  /* Setup the output pointers array */
  for (int i = 0; i < config->num_outputs; i++) {
    if (i >= num_outputs) {
      DEBUG1_VALUELN("Too many outputs:", config->num_outputs);
      DEBUG_ERR_STATE(13);
    }
    if (outputs != NULL) {
      if (readoutputs[i].hdr.type == HMTL_OUTPUT_NONE) {
        outputs[i] = NULL;
      } else {
        outputs[i] = (output_hdr_t *)&readoutputs[i];
      }
    }
  }

  DEBUG2_COMMAND(hmtl_print_config(config, outputs));

  /* Initialize the outputs */
  for (int i = 0; i < config->num_outputs; i++) {
    void *data = NULL;
    output_hdr_t *out = (output_hdr_t *)&readoutputs[i];
    switch (out->type) {
#ifdef USE_PIXELUTIL
      case HMTL_OUTPUT_PIXELS: {
        if (pixels == NULL) continue;
        data = pixels;
        break;
      }
#endif
#ifdef USE_RS485
      case HMTL_OUTPUT_RS485: {
        if (rs485 == NULL) continue;
        data = rs485;
        break;
      }
#endif
#ifdef USE_XBEE
      case HMTL_OUTPUT_XBEE: {
        if (xbee == NULL) continue;
        data = xbee;
        break;
      }
#endif
      case HMTL_OUTPUT_RGB: {
        if (rgb_output == NULL) continue;
        memcpy(rgb_output, out, sizeof (config_rgb_t));
        break;
      }
      case HMTL_OUTPUT_VALUE: {
        if (value_output == NULL) continue;
        memcpy(value_output, out, sizeof (config_value_t));
        break;
      }
#ifdef USE_MPR121
      case HMTL_OUTPUT_MPR121: {
        if (mpr121 == NULL) continue;
        data = mpr121;
        break;
      }
#endif
    }

    if (objects) objects[i] = data;

    hmtl_setup_output(config, out, data);
    outputs_found |= (1 << out->type);
  }

  return outputs_found;
}
