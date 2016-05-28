/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * Program modules that can be executed on HTML Modules
 ******************************************************************************/

#ifndef HMTLPROGRAMS_H
#define HMTLPROGRAMS_H

#include "FastLED.h"
#include "ProgramManager.h"

/*******************************************************************************
 * HMTL Programs message formats
 */

// 1 byte value
#define HMTL_PROGRAM_NONE         0x00
#define HMTL_PROGRAM_BLINK        0x01
#define HMTL_PROGRAM_TIMED_CHANGE 0x02
#define HMTL_PROGRAM_LEVEL_VALUE  0x03
#define HMTL_PROGRAM_SOUND_VALUE  0x04
#define HMTL_PROGRAM_FADE         0x05
#define HMTL_PROGRAM_SPARKLE      0x06

#define PROGRAM_SENSOR_DATA       0x10

#define PROGRAM_BRIGHTNESS        0x30 // One-time only


/*
 * Program to blink between two colors
 */
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
void hmtl_send_blink(RS485Socket *rs485, byte *buff, byte buff_len,
		     uint16_t address, uint8_t output,
		     uint16_t on_period, uint32_t on_color,
		     uint16_t off_period, uint32_t off_color);

boolean program_blink_init(msg_program_t *msg, program_tracker_t *tracker,
                           output_hdr_t *output);
boolean program_blink(output_hdr_t *output, void *object,
                      program_tracker_t *tracker);

typedef struct {
  hmtl_program_blink_t msg;
  boolean on;
  unsigned long next_change;
} state_blink_t;


/*
 * Program which sets a color, waits, and sets another color
 */
typedef struct {
  uint32_t change_period;
  uint8_t start_value[3];
  uint8_t stop_value[3];
} hmtl_program_timed_change_t;
uint16_t hmtl_program_timed_change_fmt(byte *buffer, uint16_t buffsize,
				       uint16_t address, uint8_t output,
				       uint32_t change_period,
				       uint32_t start_color,
				       uint32_t stop_color);
void hmtl_send_timed_change(RS485Socket *rs485, byte *buff, byte buff_len,
			    uint16_t address, uint8_t output,
			    uint32_t change_period,
			    uint32_t start_color,
			    uint32_t stop_color);

typedef struct {
  hmtl_program_timed_change_t msg;
  unsigned long change_time;
} state_timed_change_t;

boolean program_timed_change_init(msg_program_t *msg,
                                  program_tracker_t *tracker,
                                  output_hdr_t *output);
boolean program_timed_change(output_hdr_t *output, void *object,
                             program_tracker_t *tracker);

/*
 * Program which sets a color and fades to another over a set period
 */
typedef struct {
  uint32_t period;
  uint8_t start_value[3];
  uint8_t stop_value[3];
  uint8_t flags;
} hmtl_program_fade_t;
#define HMTL_FADE_FLAG_CYCLE 0x1
uint16_t hmtl_program_fade_fmt(byte *buffer, uint16_t buffsize,
                               uint16_t address, uint8_t output,
                               uint32_t period,
                               uint32_t start_color,
                               uint32_t stop_color,
                               uint8_t flags);
boolean program_fade_init(msg_program_t *msg, program_tracker_t *tracker,
                          output_hdr_t *output);
boolean program_fade(output_hdr_t *output, void *object,
                     program_tracker_t *tracker);

typedef struct {
  hmtl_program_fade_t msg;
  unsigned long start_time;
} state_fade_t;


/*
 * Program which generates a randomized sparkle pattern
 */
typedef struct {
  uint16_t period;        //  2B
  CRGB bgColor;           //  3B
  byte sparkle_threshold; //  1B Percentage of pixels to change each iteration
  byte bg_threshold;      //  1B Percentage of pixels to leave as background
  byte hue_max;           //  1B
  byte sat_min;           //  1B
  byte sat_max;           //  1B
  byte val_min;           //  1B
  byte val_max;           //  1B

                          // 11B Total
} hmtl_program_sparkle_t;

typedef struct {
  hmtl_program_sparkle_t msg;
  unsigned long last_change_ms;
} state_sparkle_t;

uint16_t program_sparkle_fmt(byte *buffer, uint16_t buffsize,
                           uint16_t address, uint8_t output,
                           uint32_t period,
                           CRGB bgColor);
boolean program_sparkle_init(msg_program_t *msg, program_tracker_t *tracker,
                          output_hdr_t *output);
boolean program_sparkle(output_hdr_t *output, void *object,
                     program_tracker_t *tracker);

/*
 * Program to set the brightness of a pixel output
 */
typedef struct {
  uint8_t value;
} hmtl_program_brightness_t;
uint16_t program_brightness_fmt(byte *buffer, uint16_t buffsize,
                             uint16_t address, uint8_t output,
                             uint8_t value);
boolean program_brightness(msg_program_t *msg, program_tracker_t *tracker,
                             output_hdr_t *output);



/*******************************************************************************
 * Additional helper messages
 */

// Send a request to cancel any program running on an output
void hmtl_send_cancel(RS485Socket *rs485, byte *buff, byte buff_len,
                      uint16_t address, uint8_t output);


#endif
