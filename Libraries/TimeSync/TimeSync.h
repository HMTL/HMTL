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

class TimeSync {
 public:

  TimeSync();

  unsigned long latency;
  long delta;

  /*
   * Return the current time adjusted based on the derived time delta
   */
  unsigned long ms();
  unsigned long s();
};

#endif
