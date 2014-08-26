#ifndef POOFER_H
#define POOFER_H

#include <Arduino.h>

class Poofer {
public:
  uint16_t address;

  boolean igniter_enabled;
  boolean igniter_on;
  byte igniter_switch;
  byte igniter_output;

  boolean poof_enabled;
  boolean poof_on;
  byte poof_switch;
  byte poof_output;

  Poofer(uint16_t _address, 
	 byte _igniter_switch, byte _igniter_output,
	 byte _poof_switch, byte _poof_output);

  void enableIgniter();
  void disableIgniter();
  void ignite(uint32_t period_ms);

  void enablePoof();
  void disablePoof();
  void poof(uint32_t period_ms);
};

#endif
