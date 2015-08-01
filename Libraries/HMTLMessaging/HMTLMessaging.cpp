/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#include <Arduino.h>

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_ERROR
#endif
#include "Debug.h"

#include "GeneralUtils.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "PixelUtil.h"
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
hmtl_handle_output_msg(msg_hdr_t *msg_hdr,
                       config_hdr_t *config_hdr, output_hdr_t *outputs[],
                       void *objects[])
{
  if (msg_hdr->type != MSG_TYPE_OUTPUT) {
    DEBUG_ERR("hmtl_handle_msg: incorrect msg type");
    return -1;
  }

  output_hdr_t *msg = (output_hdr_t *)(msg_hdr + 1);
  DEBUG4_VALUE("hmtl_handle_msg: type=", msg->type);
  DEBUG4_VALUELN(" out=", msg->output);

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
        uint8_t values[3];
        for (byte i = 0; i < 3; i++) {
          values[i] = msg2->value;
        }
        hmtl_set_output_rgb(out, data, values);
        break;
      }

    case HMTL_OUTPUT_RGB:
      {
        msg_rgb_t *msg2 = (msg_rgb_t *)msg;
        hmtl_set_output_rgb(out, data, msg2->values);
        break;
      }

    case HMTL_OUTPUT_PROGRAM:
    //        XXX - Need to add timed on program
    break;

    case HMTL_OUTPUT_PIXELS:
    // XXX - Need to do something here
    break;
  }

  return -1;
}

/* Check for HMTL formatted msg over a socket interface */
msg_hdr_t *
hmtl_socket_getmsg(Socket *socket, unsigned int *msglen, uint16_t address) {
  const byte *data;
  if (address == SOCKET_ADDR_INVALID) data = socket->getMsg(msglen);
  else data = socket->getMsg(address, msglen);
  if (data != NULL) {
    msg_hdr_t *msg_hdr = (msg_hdr_t *)data;
      
    if (*msglen < sizeof (msg_hdr_t)) {
      DEBUG1_VALUE("hmtl_socket_getmsg: msg length ", *msglen);
      DEBUG1_VALUELN(" short for header ", sizeof (msg_hdr_t));
      goto ERROR_OUT;
    }
    if (msg_hdr->length < sizeof (msg_hdr_t)) {
      DEBUG_ERR("hmtl_socket_getmsg: msg length is too short");
      goto ERROR_OUT;
    }

    if (msg_hdr->length != *msglen) {
      DEBUG1_VALUE("hmtl_socket_getmsg: msg->length ", msg_hdr->length);
      DEBUG1_VALUE(" != msglen ", *msglen);
      DEBUG1_COMMAND(
                     print_hex_string(data, *msglen)
                     );
      goto ERROR_OUT;
    }

    // TODO: Check the CRC!  Check the version!

    return msg_hdr;
  }

 ERROR_OUT:
  *msglen = 0;
  return NULL;
}

/* Backwards compatibility function for RS485 */
msg_hdr_t *
hmtl_rs485_getmsg(RS485Socket *rs485, unsigned int *msglen, uint16_t address) {
  return hmtl_socket_getmsg(rs485, msglen, address);
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
    //    DEBUG4_VALUE(" ", offset);
    //    DEBUG4_HEXVAL("-", val);

    if (offset == 0) {
      /* Wait for the start code at the beginning of the message */
      if (val != HMTL_MSG_START) {
        DEBUG1_HEXVALLN("hmtl_serial_getmsg: not start code: ",
                        val);
        continue;
      }

      /* This is probably the beginning of the message */ 
    }

    msg[offset] = val;
    offset++;

    if (offset >= sizeof (msg_hdr_t)) {
      /* We have the entire message header */

      if (msg_hdr->length < sizeof (msg_hdr_t)) {
        DEBUG_ERR("hmtl_serial_getmsg: msg length is too short");
        offset = 0;
        continue;
      }

      if (offset == msg_hdr->length) {
        /* This is a complete message */
        DEBUG4_PRINTLN("hmtl_serial_getmsg: Received complete command");
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
void hmtl_msg_fmt(msg_hdr_t *msg_hdr, uint16_t address, uint8_t length, 
                  uint8_t type, uint8_t flags = 0) {
  msg_hdr->startcode = HMTL_MSG_START;
  msg_hdr->crc = 0;
  msg_hdr->version = HMTL_MSG_VERSION;
  msg_hdr->length = length;
  msg_hdr->type = type;
  msg_hdr->flags = flags;
  msg_hdr->address = address;

#ifdef HMTL_USE_CRC
  /* Compute the CRC with the crc value with the crc value == 0 */
  msg_hdr->crc = EEPROM_crc(msg_hdr, length);
#endif
}

void hmtl_program_fmt(msg_program_t *msg_program, uint8_t output, 
                      uint8_t program, uint16_t buffsize) {
  DEBUG_COMMAND(DEBUG_ERROR,
                if (buffsize < HMTL_MSG_PROGRAM_LEN) {
                  DEBUG_ERR("hmtl_program_fmt: too small size");
                  DEBUG_ERR_STATE(1);
                }
                );

  msg_program->hdr.type = HMTL_OUTPUT_PROGRAM;
  msg_program->hdr.output = output;
  msg_program->type = program;
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

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_VALUE_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_VALUE_LEN;
}

/* Format an RGB message */
uint16_t hmtl_rgb_fmt(byte *buffer, uint16_t buffsize,
                      uint16_t address, uint8_t output, 
                      uint8_t r, uint8_t g, uint8_t b) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_rgb_t *msg_rgb = (msg_rgb_t *)(msg_hdr + 1);

  if (buffsize < HMTL_MSG_RGB_LEN) {
    DEBUG_ERR("hmtl_rgb_fmt: too small size");
    DEBUG_ERR_STATE(1);
  }

  msg_rgb->hdr.type = HMTL_OUTPUT_RGB;
  msg_rgb->hdr.output = output;
  msg_rgb->values[0] = r;
  msg_rgb->values[1] = g;
  msg_rgb->values[2] = b;

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_RGB_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_VALUE_LEN;
}

/* Format a poll response message */
uint16_t hmtl_poll_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                       byte flags, uint16_t object_type,
                       config_hdr_t *config, output_hdr_t *outputs[],
                       uint16_t recv_buffer_size) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_poll_response_t *msg_poll = (msg_poll_response_t *)(msg_hdr + 1);

  if (buffsize < HMTL_MSG_POLL_MIN_LEN) {
    DEBUG_ERR("hmtl_poll_fmt: too small size");
    DEBUG_ERR_STATE(1);
  }

  // Construct the primary data
  uint16_t len = HMTL_MSG_POLL_MIN_LEN;
  memcpy(&msg_poll->config, config, sizeof (config_hdr_t));
  msg_poll->object_type = object_type;
  msg_poll->recv_buffer_size = recv_buffer_size;
  msg_poll->msg_version = HMTL_MSG_VERSION;
  
  // TODO: Add outputs XXX

  hmtl_msg_fmt(msg_hdr, address, len, MSG_TYPE_POLL);
  msg_hdr->flags |= flags | MSG_FLAG_ACK;
  return len;
}

/*
 * Format a sensor response message.  The caller will fill in the actual sensor
 * data after the header.
 */
uint16_t hmtl_sensor_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                         uint8_t datalen, uint8_t **data_ptr) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_sensor_response_t *msg_sense = (msg_sensor_response_t *)(msg_hdr + 1);

  uint16_t len = HMTL_MSG_SENSOR_MIN_LEN + datalen;

  /* Format the message header */
  hmtl_msg_fmt(msg_hdr, address, len, MSG_TYPE_SENSOR);
  msg_hdr->flags |= MSG_FLAG_ACK;

  /* Set the data ptr to be returned */
  *data_ptr = (uint8_t *)&msg_sense->data;

  return len;
}

/* Format an address setting message */
uint16_t hmtl_set_addr_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                           uint16_t device_id, uint16_t new_address) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_set_addr_t *msg_addr = (msg_set_addr_t *)(msg_hdr + 1);

  if (buffsize < HMTL_MSG_SET_ADDR_LEN) {
    DEBUG_ERR("too small size");
    DEBUG_ERR_STATE(1);
  }

  msg_addr->device_id = device_id;
  msg_addr->address = new_address;
  
  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_SET_ADDR_LEN, MSG_TYPE_SET_ADDR);
  return HMTL_MSG_SET_ADDR_LEN;
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

  hmtl_program_fmt(msg_program, output, HMTL_PROGRAM_BLINK, buffsize);

  hmtl_program_blink_t *blink = (hmtl_program_blink_t *)msg_program->values;
  blink->on_period = on_period;
  blink->off_period = off_period;
  blink->on_value[0] = pixel_red(on_color);
  blink->on_value[1] = pixel_green(on_color);
  blink->on_value[2] = pixel_blue(on_color);
  blink->off_value[0] = pixel_red(off_color);
  blink->off_value[1] = pixel_green(off_color);
  blink->off_value[2] = pixel_blue(off_color);
  
  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;
}

/* Format a timed change program message */
uint16_t hmtl_program_timed_change_fmt(byte *buffer, uint16_t buffsize,
                                       uint16_t address, uint8_t output,
                                       uint32_t change_period,
                                       uint32_t start_color,
                                       uint32_t stop_color) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_program_t *msg_program = (msg_program_t *)(msg_hdr + 1);

  hmtl_program_fmt(msg_program, output, HMTL_PROGRAM_TIMED_CHANGE, buffsize);

  hmtl_program_timed_change_t *program = 
    (hmtl_program_timed_change_t *)msg_program->values;
  program->change_period = change_period;
  program->start_value[0] = pixel_red(start_color);
  program->start_value[1] = pixel_green(start_color);
  program->start_value[2] = pixel_blue(start_color);
  program->stop_value[0] = pixel_red(stop_color);
  program->stop_value[1] = pixel_green(stop_color);
  program->stop_value[2] = pixel_blue(stop_color);

  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;
}


/***** Wrapper functions for sending HMTL Messages ****************************/


void hmtl_send_value(RS485Socket *rs485, byte *buff, byte buff_len,
                     uint16_t address, uint8_t output, int value) {
  DEBUG5_VALUE("hmtl_send_value: addr:", address);
  DEBUG5_VALUE(" out:", output);
  DEBUG5_VALUELN(" value:", value);

  uint16_t len = hmtl_value_fmt(buff, buff_len,
                                address, output, value);
  rs485->sendMsgTo(address, buff, len);
}

void hmtl_send_rgb(RS485Socket *rs485, byte *buff, byte buff_len,
                   uint16_t address, uint8_t output, 
                   uint8_t r, uint8_t g, uint8_t b) {
  DEBUG5_VALUE("hmtl_send_rgb: addr:", address);
  DEBUG5_VALUE(" out:", output);
  DEBUG5_VALUE(" rgb:", r);
  DEBUG5_VALUE(",", g);
  DEBUG5_VALUELN(",", b);

  uint16_t len = hmtl_rgb_fmt(buff, buff_len,
                              address, output, r, g, b);
  rs485->sendMsgTo(address, buff, len);
}

void hmtl_send_blink(RS485Socket *rs485, byte *buff, byte buff_len,
                     uint16_t address, uint8_t output,
                     uint16_t on_period, uint32_t on_color,
                     uint16_t off_period, uint32_t off_color) {

  DEBUG5_VALUE("hmtl_send_blink: addr:", address);
  DEBUG5_VALUELN(" out:", output);

  uint16_t len = hmtl_program_blink_fmt(buff, buff_len,
                                        address, output,
                                        on_period, 
                                        on_color,
                                        off_period, 
                                        off_color);
  rs485->sendMsgTo(address, buff, len);
}

void hmtl_send_timed_change(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address, uint8_t output,
                            uint32_t change_period,
                            uint32_t start_color,
                            uint32_t stop_color) {
  DEBUG5_VALUE("hmtl_send_timed_change: addr:", address);
  DEBUG5_VALUELN(" out:", output);

  uint16_t len = hmtl_program_timed_change_fmt(buff, buff_len,
                                               address, output,
                                               change_period,
                                               start_color,
                                               stop_color);
  rs485->sendMsgTo(address, buff, len);
}

/* Send a sensor data request to an address */
void hmtl_send_sensor_request(RS485Socket *rs485, byte *buff, byte buff_len,
                              uint16_t address) {
  DEBUG5_VALUELN("hmtl_send_sensor_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_SENSOR, MSG_FLAG_RESPONSE);

  rs485->sendMsgTo(address, buff, len);
}

/* Send a poll request */
void hmtl_send_poll_request(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address) {
  DEBUG5_VALUELN("hmtl_poll_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_POLL, MSG_FLAG_RESPONSE);

  rs485->sendMsgTo(address, buff, len);

}

/*******************************************************************************
 * Data processing helper functions
 */

/*
 * Return the next sensor structure from a sensor message
 */
msg_sensor_data_t* hmtl_next_sensor(msg_hdr_t *msg, msg_sensor_data_t *current) {
  byte* next = NULL;

  if (current) {
    next = (byte *)current + sizeof (msg_sensor_data_t) + current->data_len;
    if (next >= (byte *)msg + msg->length) {
      next = NULL;
    }
  } else {
    if (msg->length >= sizeof (msg_hdr_t) + sizeof (msg_sensor_data_t)) {
      next = (byte *)(msg + 1);
    }
  }

  DEBUG1_COMMAND(
    if (msg->type != MSG_TYPE_SENSOR) {
      DEBUG1_VALUELN("Not a sensor msg:", msg->type);
      next = NULL;
    }

    if (next) {
      if (next + sizeof (msg_sensor_data_t) + ((msg_sensor_data_t*)next)->data_len > (byte *)msg + msg->length) {
        DEBUG1_PRINTLN("Invalid sensor message");
        next = NULL;
      }
    }
                 );

  return (msg_sensor_data_t*)next;
}
