/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
//#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "PixelUtil.h"
//#include "MPR121.h"
#include "RS485Utils.h"

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
      case HMTL_OUTPUT_MPR121:
        return sizeof (msg_program_t); // XXX: Make a MPR121 specific type
      case HMTL_OUTPUT_RS485:
        return 0;
      default:
        DEBUG_ERR("hmtl_output_size: bad output type");
        return 0;    
  }
}

/* Process an incoming message for this module */
int
hmtl_handle_msg(msg_hdr_t *msg_hdr,
                config_hdr_t *config_hdr, output_hdr_t *outputs[],
		void *objects[])
{
  output_hdr_t *msg = (output_hdr_t *)(msg_hdr + 1);
  DEBUG_VALUE(DEBUG_HIGH, "hmtl_handle_msg: type=", msg->type);
  DEBUG_VALUE(DEBUG_HIGH, " out=", msg->output);

  if (msg->output > config_hdr->num_outputs) {
    DEBUG_ERR("hmtl_handle_msg: too many outputs");
    return -1;
  }

  output_hdr_t *out = outputs[msg->output];
  void *data = (objects != NULL ? objects[msg->output] : NULL);

  switch (msg->type) {
      case HMTL_OUTPUT_VALUE:
      {
        msg_value_t *msg2 = (msg_value_t *)msg;
        switch (out->type) {
            case HMTL_OUTPUT_VALUE:
            {
              config_value_t *val = (config_value_t *)out;
              val->value = msg2->value;
              DEBUG_VALUELN(DEBUG_HIGH, " val=", msg2->value);
              break;
            }
            case HMTL_OUTPUT_PIXELS:
	    {
	      if (data) {
		PixelUtil *pixels = (PixelUtil *)data;;
		pixels->setAllRGB(msg2->value, msg2->value, msg2->value);
	      }
	      break;
	    }
	    case HMTL_OUTPUT_RGB:
	    {
	      config_rgb_t *rgb = (config_rgb_t *)out;
              DEBUG_PRINT(DEBUG_HIGH, " rgb=");
              for (int i = 0; i < 3; i++) {
                rgb->values[i] = msg2->value;
                DEBUG_VALUE(DEBUG_HIGH, " ", msg2->value);
              }
              DEBUG_PRINT(DEBUG_HIGH, ".");
              break;

	      break;
	    }
            default:
            {
              DEBUG_VALUELN(DEBUG_ERROR, "hmtl_handle_msg: invalid msg type for value output.  msg=", msg->type);
              break;
            }
        }
        break;
      }

      case HMTL_OUTPUT_RGB:
      {
        msg_rgb_t *msg2 = (msg_rgb_t *)msg;
        switch (out->type) {
            case HMTL_OUTPUT_RGB:
            {
              config_rgb_t *rgb = (config_rgb_t *)out;
              DEBUG_PRINT(DEBUG_HIGH, " rgb=");
              for (int i = 0; i < 3; i++) {
                rgb->values[i] = msg2->values[i];
                DEBUG_VALUE(DEBUG_HIGH, " ", msg2->values[i]);
              }
              DEBUG_PRINT(DEBUG_HIGH, ".");
              break;
            }
            case HMTL_OUTPUT_PIXELS:
	    {
	      if (data) {
		PixelUtil *pixels = (PixelUtil *)data;
		pixels->setAllRGB(msg2->values[0],
				  msg2->values[1],
				  msg2->values[2]);
	      }
	      break;
	    }

            default:
            {
              DEBUG_VALUELN(DEBUG_ERROR, "hmtl_handle_msg: invalid msg type for rgb output.  msg=", msg->type);
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

      case HMTL_OUTPUT_MPR121:
	// XXX - Need to do something here
        break;

      case HMTL_OUTPUT_RS485:
	// XXX - Need to do something here
        break;
  }

  return -1;
}

/* Check for HMTL formatted msg over the RS485 interface */
msg_hdr_t *
hmtl_rs485_getmsg(RS485Socket *rs485, unsigned int *msglen, uint16_t address) {
  const byte *data = rs485->getMsg(address, msglen);
  if (data != NULL) {
    msg_hdr_t *msg_hdr = (msg_hdr_t *)data;
      
    if (*msglen < sizeof (msg_hdr_t)) {
      DEBUG_VALUE(DEBUG_ERROR, "hmtl_rs485_getmsg: msg length ", *msglen);
      DEBUG_VALUELN(DEBUG_ERROR, " short for header ", sizeof (msg_hdr_t));
      goto ERROR_OUT;
    }

    if (msg_hdr->length < (sizeof (msg_hdr_t) + sizeof (output_hdr_t))) {
      DEBUG_ERR("hmtl_rs485_getmsg: msg length is too short");
      goto ERROR_OUT;
    }

    if (msg_hdr->length != *msglen) {
      DEBUG_VALUE(DEBUG_ERROR, "hmtl_rs485_getmsg: msg->length ", msg_hdr->length);
      DEBUG_VALUE(DEBUG_ERROR, " != msglen ", *msglen);
      DEBUG_COMMAND(DEBUG_ERROR,
		    print_hex_string(data, *msglen)
		    );
      goto ERROR_OUT;
    }

    // XXX: Check the CRC!  Check the version!

    return msg_hdr;
  }

 ERROR_OUT:
  *msglen = 0;
  return NULL;
}

/*
 * Read in a message structure from the serial interface
 */
boolean
hmtl_serial_getmsg(byte *msg, byte msg_len, byte *offset_ptr) 
{
  msg_hdr_t *msg_hdr = (msg_hdr_t *)&msg[0];
  byte offset = *offset_ptr;
  boolean complete = false;

  while (Serial.available()) {
    if (offset > msg_len) {
      /* Offset has exceed the buffer size, start fresh */
      offset = 0;
      DEBUG_ERR("hmtl_serial_update: exceed max msg len");
    }

    byte val = Serial.read();
    //    DEBUG_VALUE(DEBUG_HIGH, " ", offset);
    //    DEBUG_HEXVAL(DEBUG_HIGH, "-", val);

    if (offset == 0) {
      /* Wait for the start code at the beginning of the message */
      if (val != HMTL_MSG_START) {
        DEBUG_HEXVALLN(DEBUG_ERROR, "hmtl_serial_getmsg: not start code: ",
		      val);
        continue;
      }

      /* This is probably the beginning of the message */ 
    }

    msg[offset] = val;
    offset++;

    if (offset >= sizeof (msg_hdr_t)) {
      /* We have the entire message header */

      if (msg_hdr->length < (sizeof (msg_hdr_t) + sizeof (output_hdr_t))) {
	DEBUG_ERR("hmtl_serial_getmsg: msg length is too short");
	offset = 0;
	continue;
      }

      if (offset == msg_hdr->length) {
        /* This is a complete message */
	DEBUG_PRINTLN(DEBUG_HIGH, "hmtl_serial_getmsg: Received complete command");
        complete = true;

	// XXX: Check the CRC and the version
        break;
      }
    }
  }

  *offset_ptr = offset;
  return complete;
}

/******************************************************************************
 * Individual message formatting
 */

/* Initialize the message header */
void hmtl_msg_fmt(msg_hdr_t *msg_hdr, uint16_t address, uint8_t length) {
  msg_hdr->startcode = HMTL_MSG_START;
  msg_hdr->crc = 0;
  msg_hdr->version = HMTL_MSG_VERSION;
  msg_hdr->length = length;
  msg_hdr->address = address;
#ifdef HMTL_USE_CRC
  /* Complute the CRC with the crc value == 0 */
  msg_hdr->crc = EEPROM_crc(buffer, length);
#endif
}

/* Format a value message */
uint16_t hmtl_value_fmt(byte *buffer, uint16_t buffsize,
		    uint16_t address, uint8_t output, int value) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_value_t *msg_value = (msg_value_t *)(msg_hdr + 1);

  if (buffsize < HMTL_MSG_VALUE_LEN) {
    DEBUG_ERR("hmtl_value_fmt: too small size");
    DEBUG_ERR_STATE(1);
  }

  msg_value->hdr.type = HMTL_OUTPUT_VALUE;
  msg_value->hdr.output = output;
  msg_value->value = value;

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_VALUE_LEN);
  return HMTL_MSG_VALUE_LEN;
}

/* Format a blink program message */
uint16_t hmtl_program_blink_fmt(byte *buffer, uint16_t buffsize,
				uint16_t address, uint8_t output,
				uint16_t on_period,
				uint32_t on_color,
				uint16_t off_period,
				uint32_t off_color) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_program_t *msg_program = (msg_program_t *)(msg_hdr + 1);

  if (buffsize < HMTL_MSG_PROGRAM_LEN) {
    DEBUG_ERR("hmtl_program_blink_fmt: too small size");
    DEBUG_ERR_STATE(1);
  }

  msg_program->hdr.type = HMTL_OUTPUT_PROGRAM;
  msg_program->hdr.output = output;
  msg_program->type = HMTL_PROGRAM_BLINK;

  hmtl_program_blink_t *blink = (hmtl_program_blink_t *)msg_program->values;
  blink->on_period = on_period;
  blink->off_period = off_period;
  blink->on_value[0] = pixel_red(on_color);
  blink->on_value[1] = pixel_green(on_color);
  blink->on_value[2] = pixel_blue(on_color);
  blink->off_value[0] = pixel_red(off_color);
  blink->off_value[1] = pixel_green(off_color);
  blink->off_value[2] = pixel_blue(off_color);
  
  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN);
  return HMTL_MSG_PROGRAM_LEN;
}
