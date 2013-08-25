/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"

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

uint16_t hmtl_msg_size(output_hdr_t *output) 
{
  switch (output->type) {
      case HMTL_OUTPUT_VALUE:
        return sizeof (msg_value_t);
      case HMTL_OUTPUT_RGB:
        return sizeof (msg_rgb_t);
      case HMTL_OUTPUT_PROGRAM:
        return sizeof (msg_program_t);
      case HMTL_OUTPUT_PIXELS:
        return sizeof (msg_program_t);
      default:
        DEBUG_ERR(F("hmtl_output_size: bad output type"));
        return 0;    
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

/* Initialized the pins of an output */
int hmtl_setup_output(output_hdr_t *hdr, void *data)
{
  DEBUG_VALUE(DEBUG_HIGH, "setup_output: type=", hdr->type);
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        DEBUG_PRINT(DEBUG_HIGH, " value");
        pinMode(out->pin, OUTPUT);
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        DEBUG_PRINT(DEBUG_HIGH, " rgb");
        for (int j = 0; j < 3; j++) {
          pinMode(out->pins[j], OUTPUT);
        }
        break;
      }
      case HMTL_OUTPUT_PROGRAM:
      {
//        config_program_t *out = (config_program_t *)hdr;
        break;
      }
      case HMTL_OUTPUT_PIXELS:
      {
        if (data != NULL) {
          config_pixels_t *out = (config_pixels_t *)hdr;
          PixelUtil *pixels = (PixelUtil *)data;
          *pixels = PixelUtil(out->numPixels,
                              out->dataPin,
                              out->clockPin,
                              out->type);
        } else {
          DEBUG_ERR(F("Expect PixelUtil data struct for pixel configs"));
        }
        break;
      }
      default:
      {
        DEBUG_VALUELN(DEBUG_ERROR, F("Invalid type"), hdr->type);
        return -1;
      }
  }

  return 0;
}

/* Perform an update of an output */
int hmtl_update_output(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        analogWrite(out->pin, out->value);
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
        PixelUtil *pixels = (PixelUtil *)data;
        pixels->update();
        break;
      }
      default: 
      {
        DEBUG_ERR(F("hmtl_update_output: unknown type"));
        return -1;
      }
  }

  return 0;
}

/* Update the output with test data */
#define TEST_MAX_VAL 128
#define TEST_PWM_STEP  1
int hmtl_test_output(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        out->value = 255;
        //out->value = (out->value + TEST_PWM_STEP) % TEST_MAX_VAL;
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        for (int j = 0; j < 3; j++) {
          out->values[j] = (out->values[j] + TEST_PWM_STEP + j) % TEST_MAX_VAL;
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
//          config_pixels_t *out = (config_pixels_t *)hdr;
        PixelUtil *pixels = (PixelUtil *)data;
        static int currentPixel = 0;
        pixels->setPixelRGB(currentPixel, 0, 0, 0);
        currentPixel = (currentPixel + 1) % pixels->numPixels();
        pixels->setPixelRGB(currentPixel, 255, 0, 0);
        break;
      }
      default: 
      {
        DEBUG_ERR(F("hmtl_test_output: unknown type"));
        return -1;
      }

  }

  return 0;
}

/* Fill in a config with default values */
void hmtl_default_config(config_hdr_t *hdr)
{
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->version = HMTL_CONFIG_VERSION;
  hdr->address = 0;
  hdr->num_outputs = 0;
  hdr->flags = 0;
  DEBUG_VALUELN(DEBUG_LOW, F("hmtl_default_config: address="), hdr->address);
}

/* Print out details of a config */
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
  DEBUG_VALUE(0, "hmtl_print_config: mag: ", hdr->magic);
  DEBUG_VALUE(0, " version: ", hdr->version);
  DEBUG_VALUE(0, " address: ", hdr->address);
  DEBUG_VALUE(0, " outputs: ", hdr->num_outputs);
  DEBUG_VALUELN(0, " flags: ", hdr->flags);

  for (int i = 0; i < hdr->num_outputs; i++) {
    output_hdr_t *out1 = (output_hdr_t *)outputs[i];
    DEBUG_VALUE(0, "offset=", (int)out1);
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
          DEBUG_PRINTLN(0, "program");
          for (int i = 0; i < MAX_PROGRAM_VAL; i++) {
            DEBUG_VALUELN(0, " val=", out2->values[i]);
          }
          break;
        }
        case HMTL_OUTPUT_PIXELS:
        {
          config_pixels_t *out2 = (config_pixels_t *)out1;
          DEBUG_VALUE(0, "pixels clock=", out2->clockPin);
          DEBUG_VALUE(0, " data=", out2->dataPin);
          DEBUG_VALUE(0, " num=", out2->numPixels);
          DEBUG_VALUELN(0, " type=", out2->type);
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

/* Process an incoming message for this module */
int
hmtl_handle_msg(msg_hdr_t *msg_hdr,
                config_hdr_t *config_hdr, output_hdr_t *outputs[])
{
  output_hdr_t *msg = (output_hdr_t *)(msg_hdr++);
  DEBUG_VALUE(DEBUG_HIGH, F("hmtl_handle_msg: type="), msg->type);
  DEBUG_VALUE(DEBUG_HIGH, F(" out="), msg->output);

  if (msg->output > config_hdr->num_outputs) {
    DEBUG_ERR(F("hmtl_handle_msg: too many outputs"));
    return -1;
  }

  output_hdr_t *out = outputs[msg->output];

  switch (msg->type) {
      case HMTL_OUTPUT_VALUE:
      {
        msg_value_t *msg2 = (msg_value_t *)msg_hdr;
        switch (out->type) {
            case HMTL_OUTPUT_VALUE:
            {
              config_value_t *val = (config_value_t *)out;
              val->value = msg2->value;
              DEBUG_VALUELN(DEBUG_HIGH, F(" val="), msg2->value);
              break;
            }
            default:
            {
              DEBUG_VALUELN(DEBUG_ERROR, F("hmtl_handle_msg: invalid msg type for value output.  msg="), msg->type);
              break;
            }
        }
        break;
      }

      case HMTL_OUTPUT_RGB:
      {
        msg_rgb_t *msg2 = (msg_rgb_t *)msg_hdr;
        switch (out->type) {
            case HMTL_OUTPUT_RGB:
            {
              config_rgb_t *rgb = (config_rgb_t *)out;
              DEBUG_PRINT(DEBUG_HIGH, " rgb=");
              for (int i = 0; i < 3; i++) {
                rgb->values[i] = msg2->values[i];
                DEBUG_VALUE(DEBUG_HIGH, F(" "), msg2->values[i]);
              }
              DEBUG_PRINT(DEBUG_HIGH, ".");
              break;
            }

//            XXX - Add handling of fade or other programs

            default:
            {
              DEBUG_VALUELN(DEBUG_ERROR, F("hmtl_handle_msg: invalid msg type for rgb output.  msg="), msg->type);
              break;
            }

        }
        break;
      }

      case HMTL_OUTPUT_PROGRAM:
//        XXX - Need to add timed on program
        break;

      case HMTL_OUTPUT_PIXELS:

        break;
  }

  return -1;
}

/* Send this message to the destination module */
int
hmtl_transmit_msg(msg_hdr_t *msg_hdr) 
{
//  XXX
  return -1;
}

/*
 * Read in a message structure from the serial interface
 */
boolean
hmtl_serial_getmsg(byte *msg, byte msg_len, byte *offset_ptr) 
{
  msg_hdr_t *msg_hdr = (msg_hdr_t *)&msg[0];
  byte offset = *offset_ptr;
  boolean complete;

  while (Serial.available()) {
    if (offset > msg_len) {
      /* Offset has exceed the buffer size, start fresh */
      offset = 0;
      DEBUG_ERR(F("hmtl_serial_update: exceed max msg len"));
    }

    byte val = Serial.read();

    if (offset == 0) {
      /* Wait for the start code at the beginning of the message */
      if (val != HMTL_MSG_START) {
        DEBUG_ERR(F("hmtl_serial_update: not start code"));
        continue;
      }

      /* This is probably the beginning of the message */ 
    }

    msg[offset] = val;
    offset++;

    if (msg_hdr->length < (sizeof (msg_hdr_t) + sizeof (output_hdr_t))) {
      DEBUG_ERR(F("hmtl_serial_update: msg lenghth is too short"));
      offset = 0;
      continue;
    }

    if (offset >= sizeof (msg_hdr_t)) {
      /* We have the entire message header */

      if (offset == msg_hdr->length) {
        /* This is a complete message */
        complete = true;
        break;
      }
    }
  }

  *offset_ptr = offset;
  return complete;
}

/* Update configs based on serial commands */
int
hmtl_serial_update(config_hdr_t *config_hdr, output_hdr_t *outputs[]) 
{
  static byte msg[HMTL_MAX_MSG_LEN];
  static byte offset = 0;
  int read = 0;

  msg_hdr_t *msg_hdr = (msg_hdr_t *)&msg[0];

  while (Serial.available()) {
    if (offset > HMTL_MAX_MSG_LEN) {
      /* Offset has exceed the buffer size, start fresh */
      offset = 0;
      DEBUG_ERR(F("hmtl_serial_update: exceed max msg len"));
    }

    byte val = Serial.read();

    if (offset == 0) {
      /* Wait for the start code at the beginning of the message */
      if (val != HMTL_MSG_START) {
        DEBUG_ERR(F("hmtl_serial_update: not start code"));
        continue;
      }

      /* This is probably the beginning of the message */ 
    }

    msg[offset] = val;
    offset++;
    read++;

    if (msg_hdr->length < (sizeof (msg_hdr_t) + sizeof (output_hdr_t))) {
      DEBUG_ERR(F("hmtl_serial_update: msg lenghth is too short"));
      offset = 0;
      continue;
    }

    if (offset >= sizeof (msg_hdr_t)) {
      /* We have the entire message header */

      if (offset == msg_hdr->length) {
        /* This is a complete message */

        // XXX - This is where we'd add CRC comparison

        if (msg_hdr->address == config_hdr->address) {
          /* The message is for this address, process it */
          hmtl_handle_msg(msg_hdr, config_hdr, outputs);
        } else if (config_hdr->flags & HMTL_FLAG_MASTER) {
          /*
           * We are the master node and this message is not for us,
           * retransmit it.
           */
          hmtl_transmit_msg(msg_hdr);
        } else {
          DEBUG_ERR(F("hmtl_serial_update: not master, msg not for us"));
        }
        
        /* Reset the offset to start on a new message */
        offset = 0;
      }
    }
  }

  return read;
}
