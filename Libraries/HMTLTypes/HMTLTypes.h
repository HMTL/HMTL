/*
 * This header contains data types for HMTL modules
 */
#ifndef HMTLTYPES_H
#define HMTLTYPES_H

#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

/******************************************************************************
 * Module configuration
 */

#define HMTL_MAX_OUTPUTS 8 // The maximum number of outputs for a module

#define HMTL_CONFIG_ADDR  0x0E
#define HMTL_CONFIG_MAGIC 0x5C
#define HMTL_CONFIG_VERSION 3
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

typedef struct {
  // Fixed portion, must not change between versions
  uint8_t     magic;
  uint8_t     protocol_version;
  // End of fixed portion
  
  uint8_t     hardware_version;
  uint8_t     baud;

  uint8_t     num_outputs;
  uint8_t     flags;

  uint16_t    device_id;
  uint16_t    address;
} config_hdr_v3_t;

#if HMTL_CONFIG_VERSION == 3
  typedef config_hdr_v3_t config_hdr_t;
#elif HMTL_CONFIG_VERSION == 2
  typedef config_hdr_v2_t config_hdr_t;
#elif HMTL_CONFIG_VERSION == 1
  typedef config_hdr_v1_t config_hdr_t;
#endif

#define HMTL_NO_ADDRESS (uint16_t)-1

// Convert a 8bit baud value to actual baud
#define BYTE_TO_BAUD(val) ((uint32_t)val * 1200)
#define BAUD_TO_BYTE(val) (val / 1200)

#define HMTL_OUTPUT_NONE    (uint8_t)-1
#define HMTL_OUTPUT_VALUE   0x1
#define HMTL_OUTPUT_RGB     0x2
#define HMTL_OUTPUT_PROGRAM 0x3
#define HMTL_OUTPUT_PIXELS  0x4
#define HMTL_OUTPUT_MPR121  0x5
#define HMTL_OUTPUT_RS485   0x6

#define HMTL_FLAG_MASTER 0x1
#define HMTL_FLAG_SERIAL 0x2

typedef struct {
  byte type;
  byte output;
} output_hdr_t;

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

#define MAX_PROGRAM_VAL 12
typedef struct {
  output_hdr_t hdr;
  int values[MAX_PROGRAM_VAL];
} config_program_t; // XXX: Is this necessary?  Possibly for autodiscovery?

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

int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs);

int32_t hmtl_setup(config_hdr_t *config, 
		     config_max_t readoutputs[], output_hdr_t *outputs[], 
		     void *objects[], byte num_outputs, 
		     RS485Socket *rs485, PixelUtil *pixels, MPR121 *mpr121,
		     config_rgb_t *rgb_output, config_value_t *value_output,
		     int *configOffset);

int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[]);
void hmtl_default_config(config_hdr_t *hdr);
int hmtl_setup_output(output_hdr_t *hdr, void *data);
int hmtl_update_output(output_hdr_t *hdr, void *data);
int hmtl_test_output(output_hdr_t *hdr, void *data);
int hmtl_test_output_car(output_hdr_t *hdr, void *data);

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
