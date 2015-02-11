/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#ifndef HMTLMESSAGING_H
#define HMTLMESSAGING_H

#include "RS485Utils.h"
#include "HMTLTypes.h"

// Uncomment this line to enable CRC checking of messages
//#define HMTL_USE_CRC

/******************************************************************************
 * Transport-agnostic message types
 */

/*
 * A message is composed of a msg_hdr_t followed by additional structures
 * depending on the header's type and flag fields.
 *
 * Message header:
 * 8B:  |startcode |   crc    | version  | length   |
 *      |  type    |  flags   |       address       |
 *
 * Output message adds output_hdr_t + output-type specific data
 * 2B:  |   type   |  output  | ...
 */

#define HMTL_MAX_MSG_LEN 128

#define HMTL_MSG_START 0xFC

#define HMTL_MSG_VERSION 2
typedef struct {
  uint8_t startcode;
  uint8_t crc;
  uint8_t version;
  uint8_t length; // Length includes

  uint8_t type;
  uint8_t flags;

  // This address is redundant with the address in the RS485 socket header,
  // however it is necessary for messages received from other sources (such as
  // serial).
  uint16_t address;

} msg_hdr_t;

/* Message type codes */
#define MSG_TYPE_OUTPUT   0x1
#define MSG_TYPE_POLL     0x2
#define MSG_TYPE_SET_ADDR 0x3
#define MSG_TYPE_SENSOR   0x4

/* Message flags */
#define MSG_FLAG_ACK        0x1 // This message is an acknowledgement
#define MSG_FLAG_RESPONSE   0x2 // This message expects a response

/*******************************************************************************
 * Message formats for messages of type MSG_TYPE_OUTPUT
 */

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

/*******************************************************************************
 * Message format for MSG_TYPE_POLL
 */

typedef struct {
  config_hdr_t config;
  uint16_t object_type;
  uint16_t recv_buffer_size;
  uint8_t msg_version;
  uint8_t data[0];
} msg_poll_response_t;
#define HMTL_MSG_POLL_MIN_LEN (sizeof (msg_hdr_t) + sizeof (msg_poll_response_t))

/*******************************************************************************
 * Message format for MSG_TYPE_SET_ADDR
 */

typedef struct {
  uint16_t device_id;
  uint16_t address;
} msg_set_addr_t;
#define HMTL_MSG_SET_ADDR_LEN (sizeof (msg_hdr_t) + sizeof (msg_set_addr_t))

/*******************************************************************************
 * Message format for MSG_TYPE_SENSOR
 */
typedef struct {
  uint8_t data[];
} msg_sensor_response_t;
#define HMTL_MSG_SENSOR_MIN_LEN (sizeof (msg_hdr_t) + sizeof (msg_sensor_response_t))

typedef struct {
  uint8_t sensor_type;
  uint8_t data_len;
  uint8_t data[];
} msg_sensor_data_t;

// Sensor types
#define HMTL_SENSOR_SOUND 0x1
#define HMTL_SENSOR_LIGHT 0x2
#define HMTL_SENSOR_POT   0x3

/* This should be the largest individual message object ***********************/
typedef msg_program_t msg_max_t;

/*******************************************************************************
 * Utility functions
 */
uint16_t hmtl_msg_size(output_hdr_t *output);

/* Process a HMTL formatted message */
int hmtl_handle_output_msg(msg_hdr_t *msg_hdr,
			   config_hdr_t *config_hdr, output_hdr_t *outputs[],
			   void *objects[] = NULL);


/* Receive a message over the serial interface */
boolean hmtl_serial_getmsg(byte *msg, byte msg_len, byte *offset_ptr);

/* Receive a message over the rs485 interface */
msg_hdr_t *hmtl_rs485_getmsg(RS485Socket *rs485, unsigned int *msglen,
			     uint16_t address);


/*******************************************************************************
 * Formatting for individual messages
 */
uint16_t hmtl_value_fmt(byte *buffer, uint16_t buffsize,
			uint16_t address, uint8_t output, int value);
uint16_t hmtl_rgb_fmt(byte *buffer, uint16_t buffsize,
		      uint16_t address, uint8_t output, 
		      uint8_t r, uint8_t g, uint8_t b);
uint16_t hmtl_poll_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                       byte flags, uint16_t object_type,
                       config_hdr_t *config, output_hdr_t *outputs[],
                       uint16_t recv_buffer_size);
uint16_t hmtl_set_addr_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                           uint16_t device_id, uint16_t new_address);
uint16_t hmtl_sensor_fmt(byte *buffer, uint16_t buffsize, uint16_t address,
                         uint8_t datalen, uint8_t **data_ptr);

/*******************************************************************************
 * HMTL Programs
 */

#define HMTL_PROGRAM_NONE         0x0
#define HMTL_PROGRAM_BLINK        0x1
#define HMTL_PROGRAM_TIMED_CHANGE 0x2

/* Program to blink between two colors */
typedef struct {
  uint16_t on_period;
  uint8_t on_value[3];
  uint16_t off_period;
  uint8_t off_value[3];
} hmtl_program_blink_t;
uint16_t hmtl_program_blink_fmt(byte *buffer, uint16_t buffsize,
				uint16_t address, uint8_t output,
				uint16_t on_period,
				uint32_t on_color,
				uint16_t off_period,
				uint32_t off_color);

/* Program which sets a color, waits, and sets another color */
typedef struct {
  uint32_t change_period;
  uint8_t start_value[3];
  uint8_t stop_value[3];
} hmtl_program_timed_change_t;
uint16_t hmtl_program_timed_change_fmt(byte *buffer, uint16_t buffsize,
				       uint16_t address, uint8_t output,
				       uint16_t change_period,
				       uint32_t start_color,
				       uint32_t stop_color);

/*******************************************************************************
 * Wrapper functions for sending HMTL Messages 
 */
void hmtl_send_value(RS485Socket *rs485, byte *buff, byte buff_len,
		     uint16_t address, uint8_t output, int value);
void hmtl_send_rgb(RS485Socket *rs485, byte *buff, byte buff_len,
		   uint16_t address, uint8_t output, 
		   uint8_t r, uint8_t g, uint8_t b);
void hmtl_send_blink(RS485Socket *rs485, byte *buff, byte buff_len,
		     uint16_t address, uint8_t output,
		     uint16_t on_period, uint32_t on_color,
		     uint16_t off_period, uint32_t off_color);
void hmtl_send_timed_change(RS485Socket *rs485, byte *buff, byte buff_len,
			    uint16_t address, uint8_t output,
			    uint32_t change_period,
			    uint32_t start_color,
			    uint32_t stop_color);

void hmtl_send_poll_request(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address);

void hmtl_send_sensor_request(RS485Socket *rs485, byte *buff, byte buff_len,
                              uint16_t address);

#endif
