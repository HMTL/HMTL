/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * This library handles communications to and between HMTL Modules
 ******************************************************************************/

#include <Arduino.h>

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_ERROR
#endif
#include "Debug.h"

#include "GeneralUtils.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "HMTLPrograms.h"

#include "PixelUtil.h"
#include "RS485Utils.h"

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


/*******************************************************************************
 * Wrapper functions for sending HMTL program messages
 */


/* Send a message that clears any program for the output */
void hmtl_send_cancel(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address, uint8_t output) {
  DEBUG5_VALUE("hmtl_send_cancel: addr:", address);
  DEBUG5_VALUELN(" out:", output);

  uint16_t len = hmtl_program_cancel_fmt(buff, buff_len,
                                         address, output);
  rs485->sendMsgTo(address, buff, len);
}

void hmtl_send_blink(RS485Socket *rs485, byte *buff, byte buff_len,
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
  rs485->sendMsgTo(address, buff, len);
}

void hmtl_send_timed_change(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address, uint8_t output,
                            uint32_t change_period,
                            uint32_t start_color,
                            uint32_t stop_color) {
  DEBUG5_VALUE("hmtl_send_timed_change: addr:", address);
  DEBUG5_VALUELN(" out:", output);

  uint16_t len = hmtl_program_timed_change_fmt(buff, buff_len,
                                               address, output,
                                               change_period,
                                               start_color,
                                               stop_color);
  rs485->sendMsgTo(address, buff, len);
}

/* Send a sensor data request to an address */
void hmtl_send_sensor_request(RS485Socket *rs485, byte *buff, byte buff_len,
                              uint16_t address) {
  DEBUG5_VALUELN("hmtl_send_sensor_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_SENSOR, MSG_FLAG_RESPONSE);

  rs485->sendMsgTo(address, buff, len);
}

/* Send a poll request */
void hmtl_send_poll_request(RS485Socket *rs485, byte *buff, byte buff_len,
                            uint16_t address) {
  DEBUG5_VALUELN("hmtl_poll_request: addr:", address);

  uint16_t len = sizeof (msg_hdr_t);
  msg_hdr_t *msg = (msg_hdr_t *)buff;
  hmtl_msg_fmt(msg, address, len, MSG_TYPE_POLL, MSG_FLAG_RESPONSE);

  rs485->sendMsgTo(address, buff, len);

}
