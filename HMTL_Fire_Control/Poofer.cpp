/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 ******************************************************************************/

#define DEBUG_LEVEL DEBUG_TRACE
#include <Debug.h>

#include <Arduino.h>

#include "Poofer.h"
#include "HMTL_Fire_Control.h"

Poofer::Poofer(byte _id, uint16_t _address, 
	       byte _igniter_switch, byte _igniter_output,
	       byte _poof_switch, byte _poof_output) {
  id = _id;
  address = _address;
  igniter_switch = _igniter_switch;
  poof_switch = _poof_switch;

  igniter_output = _igniter_output;
  poof_output = _poof_output;

  igniter_enabled = false;
  igniter_on = false;
  poof_enabled = false;
  poof_on = false;

  poof_ready = false;

  igniter_off_ms = 0;
  poof_off_ms = 0;

  changed = false;
}

void Poofer::enableIgniter() {
  DEBUG_VALUELN(DEBUG_MID, "Igniter enabled: ", id);
  igniter_enabled = true;
  changed = true;
  poof_ready = false;
  ignite(POOFER_IGNITE_PERIOD);
}

void Poofer::disableIgniter() {
  DEBUG_VALUELN(DEBUG_MID, "Igniter disabled: ", id);
  igniter_enabled = false;
  igniter_on = false;
  changed = true;
  sendHMTLTimedChange(address, igniter_output, 100, 0, 0);
}


void Poofer::enablePoof() {
  DEBUG_VALUELN(DEBUG_MID, "Poof enabled: ", id);
  poof_enabled = true;
  changed = true;
}

void Poofer::disablePoof() {
  DEBUG_VALUELN(DEBUG_MID, "Poof disabled: ", id);
  poof_enabled = false;
  poof_on = false;
  changed = true;
  sendHMTLTimedChange(address, poof_output, 100, 0, 0);
}

void Poofer::ignite(uint32_t period_ms) {
  DEBUG_VALUE(DEBUG_MID, "Igniter ", id);
  DEBUG_VALUELN(DEBUG_MID, " on for:", period_ms);
  if (!igniter_on) changed = true;
  igniter_on = true;
  igniter_off_ms = millis() + period_ms;
  sendHMTLTimedChange(address, igniter_output, period_ms, 0xFFFFFFFF, 0);
}

void Poofer::poof(uint32_t period_ms) {
  if (poof_ready) {
    DEBUG_VALUE(DEBUG_MID, "Poof ", id);
    DEBUG_VALUELN(DEBUG_MID, " on for:", period_ms);
    if (!poof_on) changed = true;
    poof_on = true;
    poof_off_ms = millis() + period_ms;
    sendHMTLTimedChange(address, poof_output, period_ms, 0xFFFFFFFF, 0);
  }
}

uint32_t Poofer::ignite_remaining() {
  if (igniter_off_ms) {
    return (igniter_off_ms - millis());
  } else {
    return 0;
  }
}

void Poofer::update() {
  uint32_t now = millis();

  if (igniter_on && (now > igniter_off_ms)) {
    DEBUG_VALUELN(DEBUG_MID, "Igniter off: ", id);
    igniter_on = false;
    changed = true;
    poof_ready = true;
  }

  if (poof_on && (now > poof_off_ms)) {
    DEBUG_VALUELN(DEBUG_MID, "Poof off: ", id);
    poof_on = false;
    changed = true;
  }
}
