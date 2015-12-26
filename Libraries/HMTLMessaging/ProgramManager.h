/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 *
 * This provides a class to configure, track the state of, and execute simple
 * program functions.
 ******************************************************************************/

#ifndef PROGRAMMANAGER_H
#define PROGRAMMANAGER_H

#include "HMTLMessaging.h"
#include "TimeSync.h"

extern TimeSync time;


/*******************************************************************************
 * Function prototypes for a HMTL program and program configuration
 */
typedef struct program_tracker program_tracker_t;

typedef boolean (*hmtl_program_func)(output_hdr_t *output,
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
 * Program tracking, configuration, etc
 */
class ProgramManager {
 public:
  ProgramManager();
  ProgramManager(output_hdr_t **_outputs,
                 program_tracker_t **_trackers,
                 void **_objects,
                 byte _num_outputs,
                 hmtl_program_t *_functions, byte _num_programs);

  boolean handle_msg(msg_program_t *msg);

  boolean run_program(byte type, void *arg);

  boolean run();

  TimeSync time;

  output_hdr_t **outputs;
  void **objects;
  byte num_outputs;

 private:
  void free_tracker(int index);
  hmtl_program_t *lookup_function(byte type);

  hmtl_program_t *functions;
  byte num_programs;

  program_tracker_t **trackers;
};


/*******************************************************************************
 * XXX - This should probably get its own header file
 */

// TODO: Rather than a monolithic process_msg function, instead provide
//       an array of handler functions for each message type
typedef boolean (*msg_handler_func)(Socket *src, msg_hdr_t *msg);
typedef struct {
  uint8_t type;
  msg_handler_func function;
} msg_handler_t;

/*
 * This class is for processing socket messages
 */
class MessageHandler {
 public:
  MessageHandler();
  MessageHandler(socket_addr_t _address, ProgramManager *_manager);

  /* 
   * Process a single message
   *  src -> Socket the message came in on, or NULL if message was from the
   *    serial port.
   *  serial_socket -> If a response to the Serial device is required then this
   *    socket's data buffer is used to construct the response.
   *  config -> The device configuration
   */
  boolean process_msg(msg_hdr_t *msg_hdr, Socket *src, Socket *serial_socket,
                      config_hdr_t *config);

 private:
  ProgramManager *manager;
  socket_addr_t address;
};

#endif
