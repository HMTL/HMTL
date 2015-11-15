/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * Program modules that can be executed on HTML Modules
 ******************************************************************************/

#ifndef HMTLPROGRAMS_H
#define HMTLPROGRAMS_H

#include "TimeSync.h"
extern TimeSync time;

/*******************************************************************************
 * Function prototypes for a HMTL program and program configuration
 */
typedef struct program_tracker program_tracker_t;

typedef boolean (*hmtl_program_func)(output_hdr_t *outputs,
                                     void *object,
                                     program_tracker_t *tracker);
typedef boolean (*hmtl_program_setup)(msg_program_t *msg,
                                      program_tracker_t *tracker);

typedef struct {
  byte type;
  hmtl_program_func program;
  hmtl_program_setup setup;
} hmtl_program_t;

/* Structure used to track the state of currently active programs */
#define PROGRAM_TRACKER_DONE 0x1
struct program_tracker {
  hmtl_program_t *program;
  void *state;
  byte flags;
};


/*******************************************************************************
 * HMTL Programs message formats
 */

#define HMTL_PROGRAM_NONE         0x0
#define HMTL_PROGRAM_BLINK        0x1
#define HMTL_PROGRAM_TIMED_CHANGE 0x2
#define HMTL_PROGRAM_LEVEL_VALUE  0x3
#define HMTL_PROGRAM_SOUND_VALUE  0x4
#define HMTL_PROGRAM_FADE         0x5

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

boolean program_blink_init(msg_program_t *msg, program_tracker_t *tracker);
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
                                  program_tracker_t *tracker);
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
boolean program_fade_init(msg_program_t *msg, program_tracker_t *tracker);
boolean program_fade(output_hdr_t *output, void *object,
                     program_tracker_t *tracker);

typedef struct {
  hmtl_program_fade_t msg;
  unsigned long start_time;
} state_fade_t;


/*******************************************************************************
 * Additional helper messages
 */

// Send a request to cancel any program running on an output
void hmtl_send_cancel(RS485Socket *rs485, byte *buff, byte buff_len,
                      uint16_t address, uint8_t output);


#endif
