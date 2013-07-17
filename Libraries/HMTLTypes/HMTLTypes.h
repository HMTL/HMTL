/*
 * This header contains data types for HMTL modules
 */
#ifndef HMTLTYPES_H
#define HMTLTYPES_H

/******************************************************************************
 * Transport-agnostic message types
 */

/*
 * Generic message to set a given pin to the indicated value
 */
typedef struct {
  byte output;
  byte value;
} msg_output_value_t;

/******************************************************************************
 * Module configuration
 */

#define HMTL_CONFIG_ADDR  0x0F

#define HMTL_CONFIG_MAGIC 0x5C
typedef struct {
  uint8_t     magic;
  uint8_t     address;
  uint16_t output_address;
  uint8_t     num_outputs;
} config_hdr_t;

#define HMTL_OUTPUT_ONOFF 0x1
#define HMTL_OUTPUT_PWM   0x2
#define HMTL_OUTPUT_RGB   0x3
#define HMTL_OUTPUT_SENSE 0x4
typedef struct {
  byte type;
  byte pin[3];
} config_output_t;

int hmtl_read_config(config_hdr_t *hdr);
int hmtl_write_config(config_hdr_t *hdr);
void hmtl_default_config(config_hdr_t *hdr);

#endif
