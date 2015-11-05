/*******************************************************************************
 * This class performs time synchronization between modules, providing a
 * synchonized clock API.
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 ******************************************************************************/

#ifndef TIMESYNC_H
#define TIMESYNC_H

#include "Socket.h"
#include "HMTLMessaging.h"

class TimeSync {
 public:

  static const byte STATE_IDLE         = 0;
  static const byte STATE_AWAITING_ACK = 1;
  static const byte STATE_AWAITING_SET = 2;
  static const byte STATE_SYNCED       = 3;

  TimeSync();

  /*
   * Return the current time adjusted based on the derived time delta
   */
  unsigned long ms();
  unsigned long s();
  void set(unsigned long time);

  boolean synchronize(Socket *socket,
                      socket_addr_t target,
                      msg_hdr_t *msg_hdr);
  void resynchronize(Socket *socket,
                     socket_addr_t target);
  void check(Socket *socket, socket_addr_t target);

 private:
  unsigned long latency;
  long delta;
  byte state;
  unsigned long last_msg_time;

  void sendSyncMsg(Socket *socket, socket_addr_t target, byte phase);
};

/*******************************************************************************
 * Message format for MSG_TYPE_TIMESYNC
 */
#define TIMESYNC_SYNC   0x1
#define TIMESYNC_ACK    0x2
#define TIMESYNC_SET    0x3
#define TIMESYNC_RESYNC 0x4
#define TIMESYNC_CHECK  0x5

typedef struct {
  byte sync_phase;
  unsigned long timestamp;
} msg_time_sync_t;

#endif
