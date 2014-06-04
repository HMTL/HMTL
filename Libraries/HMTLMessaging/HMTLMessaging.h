/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#ifndef HMTLMESSAGING_H
#define HMTLMESSAGING_H

#include "HMTLTypes.h"

//#define HMTL_USE_CRC

/******************************************************************************
 * Transport-agnostic message types
 */

/*
 * Message format is:
 *
 *   msg_hdr_t + outout_hdr_t + command
 *
 * 6B:  |startcode |   crc    | version  | length   |      address       |
 * 2B:  |   type   |  output  | ...
 */

#define HMTL_MAX_MSG_LEN 128

#define HMTL_MSG_START 0xFC

#define HMTL_MSG_VERSION 1
typedef struct {
  uint8_t startcode;
  uint8_t crc;
  uint8_t version;
  uint8_t length;
  uint16_t address; // XXX: Is this redudant?  Or necessary for serial and such?
} msg_hdr_t;

typedef struct {
  output_hdr_t hdr;
  int value;
} msg_value_t;
#define HMTL_MSG_VALUE_LEN (sizeof (msg_hdr_t) + sizeof (msg_value_t))

typedef struct {
  output_hdr_t hdr;
  uint8_t values[3];
} msg_rgb_t;
#define HMTL_MSG_RGB_LEN (sizeof (msg_hdr_t) + sizeof (msg_rgb_t))

typedef struct {
  output_hdr_t hdr;
  uint8_t type;
  uint8_t values[MAX_PROGRAM_VAL];
} msg_program_t;
#define HMTL_MSG_PROGRAM_LEN (sizeof (msg_hdr_t) + sizeof (msg_program_t))

typedef msg_program_t msg_max_t;

/*
 * Utility functions
 */
uint16_t hmtl_msg_size(output_hdr_t *output);

/* Process a HMTL formatted message */
int hmtl_handle_msg(msg_hdr_t *msg_hdr,
		    config_hdr_t *config_hdr, output_hdr_t *outputs[],
		    void *objects[] = NULL);


/* Receive a message over the serial interface */
boolean hmtl_serial_getmsg(byte *msg, byte msg_len, byte *offset_ptr);

/* Receive a message over the rs485 interface */
msg_hdr_t *hmtl_rs485_getmsg(RS485Socket *rs485, unsigned int *msglen,
			     uint16_t address);


/*
 * Formatting for individual messages
 */
uint16_t hmtl_value_fmt(byte *buffer, uint16_t buffsz,
		    uint16_t address, uint8_t output, int value);


#endif