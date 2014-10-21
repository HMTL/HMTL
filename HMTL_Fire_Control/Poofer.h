#ifndef POOFER_H
#define POOFER_H

#include <Arduino.h>

#define POOFER_IGNITE_PERIOD 30000

class Poofer {
public:
  uint16_t address;

  byte id;

  boolean igniter_enabled;
  boolean igniter_on;
  byte igniter_switch;
  byte igniter_output;

  boolean poof_enabled;
  boolean poof_ready;
  boolean poof_on;
  byte poof_switch;
  byte poof_output;

  boolean changed;

  Poofer(byte id, uint16_t _address, 
	 byte _igniter_switch, byte _igniter_output,
	 byte _poof_switch, byte _poof_output);

  void enableIgniter();
  void disableIgniter();
  void ignite(uint32_t period_ms);
  uint32_t ignite_remaining();

  void enablePoof();
  void disablePoof();
  void poof(uint32_t period_ms);

  void update(void);



 private:
  uint32_t igniter_off_ms;
  uint32_t poof_off_ms;
};

#endif
