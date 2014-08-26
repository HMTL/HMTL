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

Poofer::Poofer(uint16_t _address, 
	       byte _igniter_switch, byte _igniter_output,
	       byte _poof_switch, byte _poof_output) {
  address = _address;
  igniter_switch = _igniter_switch;
  poof_switch = _poof_switch;

  igniter_output = _igniter_output;
  poof_output = _poof_output;

  igniter_enabled = false;
  igniter_on = false;
  poof_enabled = false;
  poof_on = false;
}

void Poofer::enableIgniter() {
  igniter_enabled = true;
  ignite(60*1000);
}

void Poofer::disableIgniter() {
  igniter_enabled = false;
  sendHMTLTimedChange(address, igniter_output, 100, 0, 0);
}


void Poofer::enablePoof() {
  poof_enabled = true;
}

void Poofer::disablePoof() {
  poof_enabled = false;
}

void Poofer::ignite(uint32_t period_ms) {
  sendHMTLTimedChange(address, igniter_output, period_ms, 0xFFFFFFFF, 0);
}

void Poofer::poof(uint32_t period_ms) {
  sendHMTLTimedChange(address, poof_output, period_ms, 0xFFFFFFFF, 0);
}
