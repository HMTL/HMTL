/*
 * This header contains data types for HMTL modules
 */
#ifndef HMTLTYPES_H
#define HMTLTYPES_H

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

#define HMTL_MAX_MSG_LEN 128

#define HMTL_MSG_START 'K'

typedef struct {
  byte startcode;
  byte crc;
  byte length;
  byte address;
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

#define HMTL_MAX_OUTPUTS 6 // The maximum number of outputs for a module

#define HMTL_CONFIG_ADDR  0x0E
#define HMTL_CONFIG_MAGIC 0x5C
#define HMTL_CONFIG_VERSION 1
typedef struct {
  uint8_t     magic;
  uint8_t     version;
  uint8_t     address;
  uint8_t     num_outputs;
  uint8_t     flags;
} config_hdr_t;

#define HMTL_OUTPUT_VALUE   0x1
#define HMTL_OUTPUT_RGB     0x2
#define HMTL_OUTPUT_PROGRAM 0x3
#define HMTL_OUTPUT_PIXELS  0x4

#define HMTL_FLAG_MASTER 0x1
#define HMTL_FLAG_SERIAL 0x2

typedef struct {
  output_hdr_t hdr;
  byte pin;
  int value;
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

typedef config_rgb_t config_max_t; // Set to the largest output structure

uint16_t hmtl_msg_size(output_hdr_t *output);

int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs);
int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[]);
void hmtl_default_config(config_hdr_t *hdr);
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[]);
int hmtl_setup_output(output_hdr_t *hdr, void *data);
int hmtl_update_output(output_hdr_t *hdr, void *data);
int hmtl_test_output(output_hdr_t *hdr, void *data);
int hmtl_test_output_car(output_hdr_t *hdr, void *data);


int hmtl_serial_update(config_hdr_t *config_hdr, output_hdr_t *outputs[]);


#define HTML_MAX_OUT 6

class HMTL_module 
{
  uint8_t address;

  config_max_t outputs[HTML_MAX_OUT];
};


#endif
