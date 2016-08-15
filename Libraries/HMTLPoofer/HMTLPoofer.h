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

class Poofer {
public:
  static const uint32_t IGNITE_PERIOD      = 30 * 1000;
  static const uint32_t PILOT_DELAY_PERIOD =  5 * 1000;
  static const uint32_t POOF_MAX           = 10 * 1000;

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

  // State bits
  static const uint8_t CHANGED         = 0;
  static const uint8_t IGNITER_ENABLED = 1;
  static const uint8_t IGNITER_ON      = 2;
  static const uint8_t PILOT_DELAY     = 3;
  static const uint8_t PILOT_ON        = 4;
  static const uint8_t POOF_ENABLED    = 5;
  static const uint8_t POOF_READY      = 6;
  static const uint8_t POOF_ON         = 7;  

  byte state;
  boolean checkState(byte bit);

  /* 
   * Ignition and optional pilot light 
   */
  byte igniter_output;
  byte pilot_output;

  boolean igniterEnabled();
  boolean igniterOn();
  boolean pilotOn();
  void enableIgniter();
  void disableIgniter();
  void disablePilot();
  void ignite(uint32_t period_ms, uint32_t pilot_delay_ms);
  uint32_t ignite_remaining();

  /*
   * Main fire effect 
   */
  byte poof_output;

  boolean poofEnabled();
  boolean poofOn();
  boolean poofReady();
  void enablePoof();
  void disablePoof();
  void poof(uint32_t period_ms);
  void cancelPoof();

  void update(void);
  boolean checkChanged();

 private:
  uint32_t igniter_off_ms;
  uint32_t pilot_on_ms;
  uint32_t poof_off_ms;

  Socket *socket;
  byte *send_buffer;
  byte send_buffer_size;

  void sendEnable(uint8_t output);
  void sendDisable(uint8_t output);
  void sendCancel(uint8_t output);

  void sendTimedOn(uint8_t output, uint32_t period);
  void sendDelayedOn(uint8_t output, uint32_t period);

  void sendHMTLTimedChange(uint8_t output,
                           uint32_t change_period,
                           boolean start,
                           boolean stop);
  void sendHMTLEnable(uint8_t output,
                      boolean enabled);

  void enableState(byte bit);
  void disableState(byte bit);

};

#endif
