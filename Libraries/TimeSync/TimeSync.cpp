/*******************************************************************************
 * This class performs time synchronization between modules, providing a
 * synchonized clock API.
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 ******************************************************************************/

#include <Arduino.h>

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include "TimeSync.h"

TimeSync::TimeSync() {
  latency = 0;
  delta = 0;
}

/*
 * Return the current time adjusted based on the derived time delta
 */
unsigned long TimeSync::ms() {
  return millis() + delta;
}

unsigned long TimeSync::s() {
  return ms() / 1000;
}
