/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#include <Arduino.h>
#include <HMTLTypes.h>
#include <PixelUtil.h>

#ifdef DEBUG_LEVEL_HMTLPROGRAMS
  #define DEBUG_LEVEL DEBUG_LEVEL_HMTLPROGRAMS
#endif

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_ERROR
#endif
#include "Debug.h"

#include "GeneralUtils.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "HMTLPrograms.h"

#include "PixelUtil.h"
#include "Socket.h"

#include "TimeSync.h"

/*******************************************************************************
 * Message formatting for program messages
 */

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

/* Format a cancel message */
uint16_t hmtl_program_cancel_fmt(byte *buffer, uint16_t buffsize,
                                 uint16_t address, uint8_t output) {
  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_program_t *msg_program = (msg_program_t *)(msg_hdr + 1);

  hmtl_program_fmt(msg_program, output, HMTL_PROGRAM_NONE, buffsize);
  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;
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

/* Format a sparkle program message */
uint16_t program_sparkle_fmt(byte *buffer, uint16_t buffsize,
                             uint16_t address, uint8_t output,
                             uint32_t period,
                             CRGB bgColor){

  msg_hdr_t *msg_hdr = (msg_hdr_t *)buffer;
  msg_program_t *msg_program = (msg_program_t *)(msg_hdr + 1);

  hmtl_program_fmt(msg_program, output, HMTL_PROGRAM_SPARKLE, buffsize);

  hmtl_program_sparkle_t *program =
          (hmtl_program_sparkle_t *)msg_program->values;

  /* If the other values are set then don't memset */
  memset(program, 0, MAX_PROGRAM_VAL);
  program->period = period;
  program->bgColor = bgColor;


  hmtl_msg_fmt(msg_hdr, address, HMTL_MSG_PROGRAM_LEN, MSG_TYPE_OUTPUT);
  return HMTL_MSG_PROGRAM_LEN;
}

/*******************************************************************************
 * Wrapper functions for sending HMTL program messages
 */


/* Send a message that clears any program for the output */
void hmtl_send_cancel(Socket *socket, byte *buff, byte buff_len,
                            uint16_t address, uint8_t output) {
  DEBUG5_VALUE("hmtl_send_cancel: addr:", address);
  DEBUG5_VALUELN(" out:", output);

  uint16_t len = hmtl_program_cancel_fmt(buff, buff_len,
                                         address, output);
  socket->sendMsgTo(address, buff, len);
}

void hmtl_send_blink(Socket *socket, byte *buff, byte buff_len,
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
  socket->sendMsgTo(address, buff, len);
}

void hmtl_send_timed_change(Socket *socket, byte *buff, byte buff_len,
                            uint16_t address, uint8_t output,
                            uint32_t change_period,
                            uint32_t start_color,
                            uint32_t stop_color) {
  DEBUG5_VALUE("hmtl_send_timed_change: addr:", address);
  DEBUG5_VALUE(" out:", output);
  DEBUG5_VALUE(" per:", change_period);
  DEBUG5_HEXVAL(" start:", start_color);
  DEBUG5_HEXVALLN(" stop:", stop_color);

  uint16_t len = hmtl_program_timed_change_fmt(buff, buff_len,
                                               address, output,
                                               change_period,
                                               start_color,
                                               stop_color);
  socket->sendMsgTo(address, buff, len);
}

/* Send a sensor data request to an address */
void hmtl_send_sensor_request(Socket *socket, byte *buff, byte buff_len,
                              uint16_t address) {
  DEBUG5_VALUELN("hmtl_send_sensor_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_SENSOR, MSG_FLAG_RESPONSE);

  socket->sendMsgTo(address, buff, len);
}

/* Send a poll request */
void hmtl_send_poll_request(Socket *socket, byte *buff, byte buff_len,
                            uint16_t address) {
  DEBUG5_VALUELN("hmtl_poll_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_POLL, MSG_FLAG_RESPONSE);

  socket->sendMsgTo(address, buff, len);

}

/*******************************************************************************
 * Program function to turn an output on and off
 */
boolean program_blink_init(msg_program_t *msg, program_tracker_t *tracker,
                           output_hdr_t *output, void *object) {
  if ((output == NULL) || !IS_HMTL_RGB_OUTPUT(output->type)) {
    return false;
  }

  DEBUG3_PRINT("Initializing blink program state");

  state_blink_t *state = (state_blink_t *)malloc(sizeof (state_blink_t));
  tracker->flags |= PROGRAM_DEALLOC_STATE;

  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->on = false;
  state->next_change = time.ms();

  tracker->state = state;

  DEBUG3_VALUE(" on_period:", state->msg.on_period);
  DEBUG3_VALUELN(" off_period:", state->msg.off_period);

  return true;
}

boolean program_blink(output_hdr_t *output, void *object,
                      program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = time.ms();
  state_blink_t *state = (state_blink_t *)tracker->state;

  if (now >= state->next_change) {
    if (state->on) {
      // Turn off the output
      hmtl_set_output_rgb(output, object, state->msg.off_value);

      state->on = false;
      state->next_change += state->msg.off_period;
    } else {
      // Turn on the output
      hmtl_set_output_rgb(output, object, state->msg.on_value);

      state->on = true;
      state->next_change += state->msg.on_period;
    }

    DEBUG4_PRINT("Blink");
    DEBUG4_VALUE(" now:", now);
    DEBUG4_VALUE(" next:", state->next_change);
    DEBUG4_VALUE(" on: ", state->on);

    changed = true;
  }

  DEBUG_PRINT_END();

  return changed;
}

/*******************************************************************************
 * Program function which sets a value and waits for a period setting another
 * value.
 */
boolean program_timed_change_init(msg_program_t *msg,
				                          program_tracker_t *tracker,
                                  output_hdr_t *output, void *object) {
  if ((output == NULL) || !IS_HMTL_RGB_OUTPUT(output->type)) {
    return false;
  }

  DEBUG3_PRINT("Initializing timed change program");

  state_timed_change_t *state = (state_timed_change_t *)malloc(sizeof (state_timed_change_t));
  tracker->flags |= PROGRAM_DEALLOC_STATE;

  DEBUG3_VALUE(" msgsz=", sizeof (state->msg));

  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->change_time = 0;

  tracker->state = state;

  DEBUG3_VALUELN(" change_period:", state->msg.change_period);

  return true;
}

boolean program_timed_change(output_hdr_t *output, void *object,
                             program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = time.ms();
  state_timed_change_t *state = (state_timed_change_t *)tracker->state;

  if (state->change_time == 0) {
    // Set the initial color
    hmtl_set_output_rgb(output, object, state->msg.start_value);
    state->change_time = now + state->msg.change_period;
    changed = true;
  }

  if (now > state->change_time) {
    // Set the final color
    hmtl_set_output_rgb(output, object, state->msg.stop_value);

    // Disable the program
    tracker->flags |= PROGRAM_TRACKER_DONE;
    changed = true;
  }

  return changed;
}

/*******************************************************************************
 * Program to fade between two values
 */

boolean program_fade_init(msg_program_t *msg,
                          program_tracker_t *tracker,
                          output_hdr_t *output, void *object) {
  if ((output == NULL) || !IS_HMTL_RGB_OUTPUT(output->type)) {
    return false;
  }

  DEBUG3_PRINT("Initializing fade program:");

  state_fade_t *state = (state_fade_t *)malloc(sizeof (state_fade_t));
  tracker->flags |= PROGRAM_DEALLOC_STATE;
  memcpy(&state->msg, msg->values, sizeof (state->msg));
  tracker->state = state;

  DEBUG3_VALUE(" ", state->msg.period);
  DEBUG3_VALUE(" ", state->msg.start_value[0]);
  DEBUG3_VALUE(",", state->msg.start_value[1]);
  DEBUG3_VALUE(",", state->msg.start_value[2]);
  DEBUG3_VALUE(" ", state->msg.stop_value[0]);
  DEBUG3_VALUE(",", state->msg.stop_value[1]);
  DEBUG3_VALUELN(",", state->msg.stop_value[2]);

  state->start_time = 0;

  return true;
}

boolean program_fade(output_hdr_t *output, void *object,
                     program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = time.ms();
  state_fade_t *state = (state_fade_t *)tracker->state;

  if (state->start_time == 0) {
    // Set the initial color
    hmtl_set_output_rgb(output, object, state->msg.start_value);
    changed = true;
    state->start_time = now;
    DEBUG5_VALUELN("Fade ms:", now);
  } else {
    // Calculate the color at this time
    CRGB start = CRGB(state->msg.start_value[0],
                      state->msg.start_value[1],
                      state->msg.start_value[2]);
    CRGB stop = CRGB(state->msg.stop_value[0],
                     state->msg.stop_value[1],
                     state->msg.stop_value[2]);

    // TODO: There is probably a more efficient way to do this
    unsigned int elapsed = now - state->start_time;
    if (elapsed >= state->msg.period) {
      // Disable the program
      tracker->flags |= PROGRAM_TRACKER_DONE;
      elapsed = state->msg.period;
    }
    fract8 fraction = map(elapsed, 0, state->msg.period, 0, 255);

    CRGB current = blend(start, stop, fraction);
    hmtl_set_output_rgb(output, object, current.raw);
    changed = true;

    DEBUG5_VALUE("Fade ms:", now);
    DEBUG5_VALUE(" elapsed:", elapsed);
    DEBUG5_VALUELN(" fract:", fraction);
  }

  return changed;
}

/*******************************************************************************
 * Program to produce a colorful "sparkle" pattern
 */

boolean program_sparkle_init(msg_program_t *msg,
                             program_tracker_t *tracker,
                             output_hdr_t *output, void *object) {
  if ((output == NULL) || (output->type != HMTL_OUTPUT_PIXELS)) {
    return false;
  }

  DEBUG3_PRINT("Initializing sparkle program:");

  state_sparkle_t *state = (state_sparkle_t *)malloc(sizeof (state_sparkle_t));
  tracker->flags |= PROGRAM_DEALLOC_STATE;
  memcpy(&state->msg, msg->values, sizeof (state->msg));
  tracker->state = state;

  if (state->msg.sparkle_threshold == 0) state->msg.sparkle_threshold = 50;
  if (state->msg.bg_threshold == 0) state->msg.bg_threshold = 20;
  if (state->msg.hue_max == 0) state->msg.hue_max = 255;
  if (state->msg.sat_max == 0) state->msg.sat_max = 255;
  if (state->msg.val_max == 0) state->msg.val_max = 255;

  DEBUG3_VALUE(" ", state->msg.period);
  DEBUG3_VALUE(" ", state->msg.bgColor.r);
  DEBUG3_VALUE(",", state->msg.bgColor.g);
  DEBUG3_VALUE(",", state->msg.bgColor.b);
  DEBUG3_VALUE(" ", state->msg.sparkle_threshold);
  DEBUG3_VALUE(" ", state->msg.bg_threshold);
  DEBUG3_VALUE(" ", state->msg.hue_max);
  DEBUG3_VALUE(" ", state->msg.sat_min);
  DEBUG3_VALUE(" ", state->msg.sat_max);
  DEBUG3_VALUE(" ", state->msg.val_min);
  DEBUG3_VALUELN(" ", state->msg.val_max);

  // Combine the bg and sparkle threshold to get the true bgthreshold
  state->msg.bg_threshold += state->msg.sparkle_threshold;

  state->last_change_ms = millis();

  return true;
}

boolean program_sparkle(output_hdr_t *output, void *object,
                        program_tracker_t *tracker) {
  unsigned long now = time.ms();
  state_sparkle_t *state = (state_sparkle_t *)tracker->state;

  if (now - state->last_change_ms >= state->msg.period) {
    PixelUtil *pixels = (PixelUtil*)object;

    state->last_change_ms = now;

    for (PIXEL_ADDR_TYPE led = 0; led < pixels->numPixels(); led++) {
      byte rand = (byte)random(100);
      if (rand <= state->msg.sparkle_threshold) {
        CRGB color = CHSV((uint8_t)random(state->msg.hue_max),
                          state->msg.sat_min +
                                  (uint8_t)random(state->msg.sat_max - state->msg.sat_min),
                          state->msg.val_min +
                                  (uint8_t)random(state->msg.val_max - state->msg.val_min));
        pixels->setPixelRGB(led, color);
      } else if (rand <= state->msg.bg_threshold) {
        pixels->setPixelRGB(led, state->msg.bgColor);
      } // Otherwise leave as previous color
    }

    return true;
  }

  return false;
}

/*******************************************************************************
 * Program to set the brightness of a pixel output.  This is an once-only
 * command.
 */
boolean program_brightness(msg_program_t *msg, program_tracker_t *tracker,
                           output_hdr_t *output, void *object) {
  if ((output == NULL) || (output->type != HMTL_OUTPUT_PIXELS)) {
    return false;
  }

  hmtl_program_brightness_t *bright = (hmtl_program_brightness_t *)msg->values;

  DEBUG3_VALUELN("Brightness:", bright->value);
  FastLED.setBrightness(bright->value);

  return false;
}

/*
 * Set a range of LEDs to an indicated color
 */
boolean program_color(msg_program_t *msg, program_tracker_t *tracker,
                      output_hdr_t *output, void *object) {
  if ((output == NULL) || (output->type != HMTL_OUTPUT_PIXELS)) {
    return false;
  }

  hmtl_program_color_t *color = (hmtl_program_color_t *)msg->values;

  DEBUG3_VALUE("Col:", color->color);
  DEBUG3_VALUE("Ran:", color->range.start);
  DEBUG3_VALUELN("-", color->range.start + color->range.length - 1);

  PixelUtil *pixels = (PixelUtil*)object;
  pixels->setRangeRGB(color->range, color->color);

  return false;
}

/*******************************************************************************
 * Program to cycle a pattern around a pixel strip
 *
 * This program runs colors on a length of pixels that circles around the entire
 * LED array.
 */

boolean program_circular_init(msg_program_t *msg, program_tracker_t *tracker,
                              output_hdr_t *output, void *object) {
  if ((output == NULL) || (output->type != HMTL_OUTPUT_PIXELS)) {
    return false;
  }

  DEBUG3_PRINT("Initializing circular program:");

  /* Allocate a new state object */
  state_circular_t *state = (state_circular_t *)malloc(sizeof (state_circular_t));
  tracker->state = state;
  tracker->flags |= PROGRAM_DEALLOC_STATE;

  /* Copy the incoming message into the state */
  memcpy(&state->msg, msg->values, sizeof (state->msg));


  /* Set defaults if not set in the program message */
  if (state->msg.period == 0) state->msg.period = 100;
  if (state->msg.length == 0) state->msg.length = 10;

  DEBUG3_VALUE(" ", state->msg.period);
  DEBUG3_VALUE(" ", state->msg.length);
  DEBUG3_VALUE(" ", state->msg.bgColor.r);
  DEBUG3_VALUE(",", state->msg.bgColor.g);
  DEBUG3_VALUE(",", state->msg.bgColor.b);
  DEBUG3_VALUELN(" ", state->msg.pattern);

  state->last_change_ms = millis();

  return true;
}

boolean program_circular(output_hdr_t *output, void *object,
                         program_tracker_t *tracker) {
  unsigned long now = time.ms();
  state_circular_t *state = (state_circular_t *)tracker->state;

  if (now - state->last_change_ms >= state->msg.period) {
    PixelUtil *pixels = (PixelUtil *)object;

    state->last_change_ms = now;

    /* Clear the previous current LED */
    pixels->setPixelRGB(state->current, CRGB(0,0,0));

    /* Increment the current start of the colored LEDs */
    state->current = (state->current + 1) % pixels->numPixels();

    /* Set the colors of the LEDs */
    switch (state->msg.pattern) {
      default: {
        for (byte i = 0; i < state->msg.length; i++) {
          uint16_t led = (state->current + i) % pixels->numPixels();

          /* Scale the color based so that the center LED is brightest */
          byte scale = 255 -
                       abs(state->msg.length / 2 - i) *
                       (255 / (state->msg.length / 2));
          CRGB color = CRGB(CHSV(state->current, 255, 255)).nscale8(scale);
          pixels->setPixelRGB(led, color);
        }
      }
    }

    return true;
  }

  return false;
}
