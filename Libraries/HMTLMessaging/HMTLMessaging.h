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
  byte startcode;
  byte crc;
  byte version;
  byte length;
  uint16_t address; // XXX: Is this redudant?  Or necessary for serial and such?
} msg_hdr_t;

typedef struct {
  output_hdr_t hdr;
  int value;
} msg_value_t;

typedef struct {
  output_hdr_t hdr;
  byte values[3];
} msg_rgb_t;

typedef struct {
  output_hdr_t hdr;
  uint8_t type;
  byte values[MAX_PROGRAM_VAL];
} msg_program_t;

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


#endif
