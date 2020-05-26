/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 *
 * This provides a class to listen for messages over the serial port and a
 * set of socket and to process them as needed.
 ******************************************************************************/

#ifndef HMTL_MESSAGEHANDLER_H
#define HMTL_MESSAGEHANDLER_H

#include "HMTLMessaging.h"
#include "ProgramManager.h"

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
  MessageHandler(socket_addr_t _address, ProgramManager *_manager,
                 Socket *_sockets[], uint8_t _num_sockets);

  /*
   * Check if a serial-ready messages should be sent over the serial port
   */
  void serial_ready();

  /*
   * Check the serial device and all sockets for messages.
   *
   * Returns true if processing the message resulted in some change that may
   * require the device's outputs to be updated.
   */
  boolean check(config_hdr_t *config);

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
  boolean check_serial(config_hdr_t *config);

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

  ProgramManager *manager;

private:
  socket_addr_t address;
  Socket **sockets;
  uint8_t num_sockets;

  /*
   * Messages from a serial interface may come in across multiple calls to
   * check serial and so must be buffered.
   */
  static const uint8_t MSG_MAX_SZ = (sizeof(msg_hdr_t) + sizeof(msg_max_t));
  byte serial_msg[MSG_MAX_SZ];
  byte serial_msg_offset;

  /*
   * Parameters for determining if a "ready" message should be sent to the
   * serial port.
   */
  static const uint16_t READY_THRESHOLD = 10000;
  static const uint16_t READY_RESEND_PERIOD = 1000;

  unsigned long last_serial_ms;
  unsigned long last_ready_ms;


};


#endif //HMTL_MESSAGEHANDLER_H
