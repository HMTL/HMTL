/*
 * This header contains data types for HMTL modules
 */
#ifndef HMTLTYPES_H
#define HMTLTYPES_H

#include "PixelUtil.h"
#include "RS485Utils.h"

/******************************************************************************
 * Transport-agnostic message types
 */

/*
 * Generic message to set a given output to the indicated value
 */
typedef struct {
  byte output;
  byte value;
} msg_output_value_t; // XXX Ditch me

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
  uint16_t address;
} msg_hdr_t;

typedef struct {
  byte type;
  byte output;
} output_hdr_t;

typedef struct {
  output_hdr_t hdr;
  int value;
} msg_value_t;

typedef struct {
  output_hdr_t hdr;
  byte values[3];
} msg_rgb_t;

#define MAX_PROGRAM_VAL 12
typedef struct {
  output_hdr_t hdr;
  byte val[3];
  byte values[MAX_PROGRAM_VAL];
} msg_program_t;

typedef msg_program_t msg_max_t;

/******************************************************************************
 * Module configuration
 */

#define HMTL_MAX_OUTPUTS 8 // The maximum number of outputs for a module

#define HMTL_CONFIG_ADDR  0x0E
#define HMTL_CONFIG_MAGIC 0x5C
#define HMTL_CONFIG_VERSION 2
typedef struct {
  uint8_t     magic;
  uint8_t     version;
  uint8_t     address;
  uint8_t     num_outputs;
  uint8_t     flags;
} config_hdr_v1_t;

typedef struct {
  uint8_t     magic;
  uint8_t     protocol_version;
  uint8_t     hardware_version;
  uint16_t    address;
  uint8_t     reserved;

  uint8_t     num_outputs;
  uint8_t     flags;
} config_hdr_v2_t;

typedef config_hdr_v2_t config_hdr_t;

#define HMTL_NO_ADDRESS (uint16_t)-1

#define HMTL_OUTPUT_VALUE   0x1
#define HMTL_OUTPUT_RGB     0x2
#define HMTL_OUTPUT_PROGRAM 0x3
#define HMTL_OUTPUT_PIXELS  0x4
#define HMTL_OUTPUT_MPR121  0x5
#define HMTL_OUTPUT_RS485   0x6

#define HMTL_FLAG_MASTER 0x1
#define HMTL_FLAG_SERIAL 0x2

typedef struct {
  output_hdr_t hdr;
  byte pin;
  int value; // XXX - Make this byte?  PWM only goes to 2552
} config_value_t;

typedef struct {
  output_hdr_t hdr;
  byte pins[3];
  byte values[3];
} config_rgb_t;

typedef struct {
  output_hdr_t hdr;
  int values[MAX_PROGRAM_VAL];
} config_program_t;

typedef struct {
  output_hdr_t hdr;
  byte clockPin;
  byte dataPin;
  uint16_t numPixels;
  byte type;
} config_pixels_t;

// This should be MPR121::MAX_SENSORS, but we don't want to include that here
#define MAX_MPR121_PINS 12
typedef struct {
  output_hdr_t hdr;
  byte irqPin;
  boolean useInterrupt;
  byte thresholds[MAX_MPR121_PINS];
} config_mpr121_t;

typedef struct {
  output_hdr_t hdr;
  byte recvPin;
  byte xmitPin;
  byte enablePin;
} config_rs485_t;

typedef config_mpr121_t config_max_t; // Set to the largest output structure

uint16_t hmtl_msg_size(output_hdr_t *output);

int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs);

int32_t hmtl_setup(config_hdr_t *config, 
		   config_max_t readoutputs[], output_hdr_t *outputs[], 
		   void *objects[],
		   byte num_outputs, RS485Socket *rs485, PixelUtil *pixels,
		   config_rgb_t *rgb_output, config_value_t *value_output,
		   int *configOffset);

int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[]);
void hmtl_default_config(config_hdr_t *hdr);
int hmtl_setup_output(output_hdr_t *hdr, void *data);
int hmtl_update_output(output_hdr_t *hdr, void *data);
int hmtl_test_output(output_hdr_t *hdr, void *data);
int hmtl_test_output_car(output_hdr_t *hdr, void *data);

int hmtl_handle_msg(msg_hdr_t *msg_hdr,
		    config_hdr_t *config_hdr, output_hdr_t *outputs[],
		    void *objects[] = NULL);
boolean hmtl_serial_getmsg(byte *msg, byte msg_len, byte *offset_ptr);
int hmtl_serial_update(config_hdr_t *config_hdr, output_hdr_t *outputs[]);

msg_hdr_t *hmtl_rs485_getmsg(RS485Socket *rs485, unsigned int *msglen,
			     uint16_t address);

/* Configuration validation */
boolean hmtl_validate_header(config_hdr_t *config_hdr);
boolean hmtl_validate_value(config_value_t *val);
boolean hmtl_validate_rgb(config_rgb_t *rgb);
boolean hmtl_validate_pixels(config_pixels_t *pixels);
boolean hmtl_validate_mpr121(config_mpr121_t *mpr121);
boolean hmtl_validate_rs485(config_rs485_t *rs485);
boolean hmtl_validate_config(config_hdr_t *config_hdr, output_hdr_t *outputs[],
			     int num_outputs);

/* Debug printing of configuration */
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[]);
void hmtl_print_header(config_hdr_t *hdr);
void hmtl_print_output(output_hdr_t *val);

#define HTML_MAX_OUT 6

class HMTL_module 
{
  uint8_t address;

  config_max_t outputs[HTML_MAX_OUT];
};


#endif
