/*******************************************************************************
 * Class for controlling poofers via HMTL Modules 
 *
 * This class is intended to be used by a controller module which then
 * communicates with an addressable module connected directly to the flame
 * effect.
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 ******************************************************************************/

#ifndef POOFER_H
#define POOFER_H

#include <Arduino.h>
#include "Socket.h"

#define POOFER_IGNITE_PERIOD 30000

class Poofer {
public:
  static const uint32_t IGNITE_PERIOD = 30 * 1000;
  static const uint32_t PILOT_DELAY   =  5 * 1000;
  static const uint32_t POOF_MAX      = 10 * 1000;

  Poofer(byte _id, 
         uint16_t _address, 
         byte _igniter_output,
         byte _pilot_output,
         byte _poof_output,
         Socket *_socket, 
         byte *_send_buffer, 
         byte send_buffer_size);

  uint16_t address;

  byte id; // Is this needed?

  boolean changed; // TODO: This needs a method of being reset

  /* 
   * Ignition and optional pilot light 
   */
  boolean igniter_enabled;
  boolean igniter_on;
  byte igniter_output;
  boolean pilot_on;
  byte pilot_output;

  void enableIgniter();
  void disableIgniter();
  void disablePilot();
  void ignite(uint32_t period_ms, uint32_t pilot_delay_ms);
  uint32_t ignite_remaining();

  /*
   * Main fire effect 
   */
  boolean poof_enabled;
  boolean poof_ready;
  boolean poof_on;
  byte poof_output;

  void enablePoof();
  void disablePoof();
  void poof(uint32_t period_ms);

  void update(void);
  boolean checkChanged();

 private:
  uint32_t igniter_off_ms;
  uint32_t poof_off_ms;

  Socket *socket;
  byte *send_buffer;
  byte send_buffer_size;

  void sendEnable(uint8_t output);
  void sendDisable(uint8_t output);

  void sendTimedOn(uint8_t output, uint32_t period);
  void sendDelayedOn(uint8_t output, uint32_t period);

  void sendHMTLTimedChange(uint8_t output,
                           uint32_t change_period,
                           boolean start,
                           boolean stop);
  void sendHMTLEnable(uint8_t output,
                      boolean enabled);

};

#endif
