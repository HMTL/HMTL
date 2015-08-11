/*******************************************************************************
 * Class for controlling poofers via HMTL Modules 
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 ******************************************************************************/

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_TRACE
#include "Debug.h"

#include "Socket.h"

#include "HMTLPoofer.h"
#include "HMTLMessaging.h"

Poofer::Poofer(byte _id, 
               
               uint16_t _address,    // HMTL Address

               byte _igniter_output, // Module output # for the ignitor
               byte _pilot_output,   // Module output # for the pilot value
               byte _poof_output,    // Moudle output # for the main valve
               
               Socket *_socket,      
               byte *_send_buffer, 
               byte _send_buffer_size) {
  id = _id;
  address = _address;

  igniter_output = _igniter_output;
  pilot_output = _pilot_output;
  poof_output = _poof_output;

  socket = _socket;
  send_buffer = _send_buffer;
  send_buffer_size = _send_buffer_size;

  igniter_enabled = false;
  igniter_on = false;
  pilot_on = false;

  poof_enabled = false;
  poof_on = false;

  poof_ready = false;

  igniter_off_ms = 0;
  poof_off_ms = 0;

  changed = false;

  DEBUG3_VALUELN("Init poofer: ", id);
}

/********************************************************************************
 * Communication wrappers
 */

/*
 * Send a timed change message to the poofer's module
 */
void Poofer::sendHMTLTimedChange(uint8_t output,
                                 uint32_t change_period,
                                 boolean start,
                                 boolean stop) {

  uint32_t start_value = (start ? (uint32_t)-1 : 0);
  uint32_t stop_value = (stop ? (uint32_t)-1 : 0);

  // TODO: hmtl_send_timed_change is RS485 only currently
  hmtl_send_timed_change((RS485Socket *)socket, send_buffer, send_buffer_size,
                         address, 
                         output,
                         change_period,
                         start_value,
                         stop_value);
}

void Poofer::sendHMTLEnable(uint8_t output,
                            boolean enabled) {
  // TODO: Send an actual value message
  sendHMTLTimedChange(output, 100, enabled, enabled);
}

void Poofer::sendEnable(uint8_t output) {
  sendHMTLEnable(output, true);
}
void Poofer::sendDisable(uint8_t output){
  sendHMTLEnable(output, false);
}

void Poofer::sendTimedOn(uint8_t output, uint32_t period) {
  sendHMTLTimedChange(output, period, true, false);
}

void Poofer::sendDelayedOn(uint8_t output, uint32_t period) {
  sendHMTLTimedChange(output, period, false, true);
}

/********************************************************************************
 * Ignition and pilot controls
 */

void Poofer::enableIgniter() {
  DEBUG3_VALUELN("Igniter enabled:", id);
  igniter_enabled = true;

  // Disable poofing while ignition sequence is started
  poof_ready = false;

  ignite(IGNITE_PERIOD, PILOT_DELAY);

  changed = true;
}

void Poofer::disableIgniter() {
  DEBUG3_VALUELN("Igniter disabled:", id);

  sendDisable(igniter_output);

  igniter_enabled = false;
  igniter_on = false;

  changed = true;
}

/* Turn off the pilot valve and disable poofing */
void Poofer::disablePilot() {
  if (pilot_output != HMTL_NO_OUTPUT) {
    DEBUG3_VALUELN("Pilot disabled:", id);
    poof_ready = false;
    disablePoof();
    sendDisable(pilot_output);
  }
}

void Poofer::ignite(uint32_t period_ms, uint32_t pilot_delay_ms) {
  if (!igniter_enabled) {
    DEBUG_ERR("Ignite not enabled");
    return;
  }

  DEBUG3_VALUE("Igniter ", id);
  DEBUG3_VALUELN(" on for:", period_ms);

  if (!igniter_on) changed = true;

  igniter_on = true;
  igniter_off_ms = millis() + period_ms;
  sendTimedOn(igniter_output, period_ms);

  if (pilot_output != HMTL_NO_OUTPUT) {
    /*
     * If there is a pilot light valve defined then start the flow briefly
     * after engaging the ignitor
     */
    pilot_on = true;
    sendDelayedOn(poof_output, pilot_delay_ms);
    
  }
}

/* Return the remaining time in the ignition sequence */
uint32_t Poofer::ignite_remaining() {
  if (igniter_off_ms) {
    return (igniter_off_ms - millis());
  } else {
    return 0;
  }
}


/********************************************************************************
 * Poofer control
 */

void Poofer::enablePoof() {
  DEBUG3_VALUELN("Poof enabled:", id);
  poof_enabled = true;
  changed = true;
}

void Poofer::disablePoof() {
  DEBUG3_VALUELN("Poof disabled:", id);
  poof_enabled = false;
  poof_on = false;
  changed = true;
  sendDisable(poof_output);
}

/* Trigger a poof if the poofer is enabled */
void Poofer::poof(uint32_t period_ms) {
  if (poof_ready && poof_enabled) {
    DEBUG3_VALUE("Poof ", id);
    DEBUG3_VALUELN(" on for:", period_ms);

    if (!poof_on) changed = true;

    if (period_ms > POOF_MAX) {
      DEBUG_ERR("Exceeded poof max");
      period_ms = POOF_MAX;
    }

    poof_on = true;
    poof_off_ms = millis() + period_ms;

    sendTimedOn(poof_output, period_ms);
  }
}

/********************************************************************************
 * Check and update status
 */

void Poofer::update() {
  uint32_t now = millis();

  if (igniter_on && (now > igniter_off_ms)) {
    /* 
     * The ignition period has elapsed, the ignitor should have turned itself
     * off and the poofer can be enabled 
     */
    DEBUG3_VALUELN("Igniter off: ", id);
    igniter_on = false;
    changed = true;
    poof_ready = true;
    igniter_off_ms = 0;
  }

  if (poof_on && (now > poof_off_ms)) {
    /* The poof period has elapsed, the poofer should have turned itself off */
    DEBUG3_VALUELN("Poof off: ", id);
    poof_on = false;
    changed = true;
  }
}

/* Check if state has changed, and if so reset the change tracker */
boolean Poofer::checkChanged() {
  boolean retval = changed;
  changed = false;
  return retval;
}
