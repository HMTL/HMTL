/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2019
 *
 ******************************************************************************/

#include "Arduino.h"
#include "Debug.h"

#include "HMTLPrograms.h"
#include "MessageHandler.h"
#include "Programmanager.h"
#include "Socket.h"

#include "HMTL_Module.h"
#include "Module_Startup_Commands.h"

#ifndef PLATFORMIO
#define STARTUP_FADE_ALL
#endif

#ifdef STARTUP_COMMANDS
/*******************************************************************************
 * Execute any startup commands
 */
void startup_commands(ProgramManager *manager, MessageHandler *handler, 
                      Socket **sockets, config_hdr_t *config) {

  /*
   * TODO: This mechanism is overly hard-coded.  A method of adding commands in
   * the EEPROM configuration would be much better, allowing for proper
   * configuration.
   */
  msg_hdr_t *msg = NULL;

#ifdef STARTUP_VALUE
  byte num = 0;
  byte output;
  while ((output = manager->lookup_output_by_type(HMTL_OUTPUT_VALUE, num)) != HMTL_NO_OUTPUT) {
    num++;
    DEBUG4_VALUELN("Init: value ", output);
    hmtl_set_output_rgb(outputs[output], objects[output],
                        pixel_color(STARTUP_VALUE, STARTUP_VALUE, STARTUP_VALUE));
  }
#endif // STARTUP_VALUE

#ifdef STARTUP_BLINK
  // Go through the outputs and set them to blink
  byte num = 0;
  byte output;
  while ((output = manager->lookup_output_by_type(HMTL_OUTPUT_VALUE, num)) != HMTL_NO_OUTPUT) {
    DEBUG3_VALUELN("Init: blink ", output);
    num++;
    hmtl_program_blink_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                           config->address, output,
                           250*num, pixel_color(128,0,0),
                           250, 0);
    msg = (msg_hdr_t *)sockets[0]->send_buffer;
#endif // STARTUP_BLINK

#ifdef STARTUP_SPARKLE
  byte output = manager->lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  if (output != HMTL_NO_OUTPUT) {
    DEBUG4_VALUELN("Init: sparkle ", output);
    program_sparkle_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                        config->address, output,
#ifdef STARTUP_ARGS
                        STARTUP_ARGS);
#else
                        0,0,0,0,0,0,0,0,0,0);
#endif
    msg = (msg_hdr_t *)sockets[0]->send_buffer;
  }
#endif // STARTUP_SPARKLE

#ifdef STARTUP_CIRCULAR
  byte output = manager->lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  if (output != HMTL_NO_OUTPUT) {
    DEBUG4_VALUELN("Init: circular ", output);
    program_circular_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                         config->address, output,
#ifdef STARTUP_ARGS
                         STARTUP_ARGS);
#else
                         100, pixels.numPixels() / 3, CRGB::Black,
                         1, 0);
#endif
    msg = (msg_hdr_t *)sockets[0]->send_buffer;
#endif // STARTUP_CIRCULAR

#ifdef STARTUP_FADE_ALL
  uint8_t i = 0;
  uint8_t output;
  for (output = manager->lookup_output_by_type(HMTL_OUTPUT_VALUE, i++);
       output != HMTL_NO_OUTPUT;
       output = manager->lookup_output_by_type(i++)) {
    DEBUG4_VALUELN("STARTUP FADE:", output);
    hmtl_program_fade_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                          config->address, output,
                          (uint32_t)(i + 1) * 1000,
                          pixel_color(8,0,0),
                          pixel_color(1,0,0),
                          HMTL_FADE_FLAG_CYCLE);
  }

#endif


  if (msg) {
#ifdef STARTUP_BROADCAST
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (sockets[i]) handler->check_and_forward(msg, sockets[i]);
      }
#endif

    handler->process_msg(msg, sockets[0], NULL, config);
  }
}

#endif // STARTUP_COMMANDS
