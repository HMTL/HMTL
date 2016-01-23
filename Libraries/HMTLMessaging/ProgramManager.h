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
   * Check is a serial-ready messages should be sent over the serial port
   */
  void serial_ready();

  /*
   * Process a single message
   *   msg_hdr: The message to be processed
   *   src: Socket the message came in on, or NULL if message was from the
   *        serial port.
   *   serial_socket: If a response to the Serial device is required then this
   *                  socket's data buffer is used to construct the response.
   *   config: The device configuration
   *
   * Returns true if processing the message resulted in some change that may
   * require the device's outputs to be updated.
   */
  boolean process_msg(msg_hdr_t *msg_hdr, Socket *src,
                      Socket *serial_socket,
                      config_hdr_t *config);

  /*
   * Check for messages over the Serial port.  If a message is received,
   * forward it over other sockets if it isn't for this device or is a broacast
   * message, and then process the message if it is for this device.
   *
   * Returns true if processing the message resulted in some change that may
   * require the device's outputs to be updated.
   */
  boolean check_serial(Socket *sockets[], uint8_t num_sockets,
                       config_hdr_t *config);

  /*
   * Check for messages over the indicated socket and handle any messages
   * received.
   *
   * Returns true if processing the message resulted in some change that may
   * require the device's outputs to be updated.
   */
  boolean check_socket(Socket *socket,
                       Socket *serial_socket,
                       config_hdr_t *config);

  /*
   * Check if a message should be forwarded and transmit it over
   * the indicated socket if so.
   */
  boolean check_and_forward(msg_hdr_t *msg_hdr, Socket *socket);

private:
  ProgramManager *manager;
  socket_addr_t address;

  /*
   * Messages from a serial interface may come in across multiple calls to
   * check serial and so must be buffered.
   */
  static const uint8_t MSG_MAX_SZ = (sizeof(msg_hdr_t) + sizeof(msg_max_t));
  byte serial_msg[MSG_MAX_SZ];
  byte serial_msg_offset;

  static const uint16_t READY_THRESHOLD = 10000;
  static const uint16_t READY_RESEND_PERIOD = 1000;

  unsigned long last_serial_ms;
  unsigned long last_ready_ms;

};

#endif
