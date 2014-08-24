/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 ******************************************************************************/

#define DEBUG_LEVEL DEBUG_TRACE
#include <Debug.h>

#include <Arduino.h>
#include <NewPing.h>
#include <Wire.h>
#include "MPR121.h"

#include "HMTL_Fire_Control.h"

boolean data_changed = true;

/******* Switches *************************************************************/

#define NUM_SWITCHES 4
boolean switch_states[NUM_SWITCHES] = { false, false, false, false };
boolean switch_changed[NUM_SWITCHES] = { false, false, false, false };
const byte switch_pins[NUM_SWITCHES] = {
  SWITCH_PIN_1, SWITCH_PIN_2, SWITCH_PIN_3, SWITCH_PIN_4 };

void initialize_switches(void) {
  for (byte i = 0; i < NUM_SWITCHES; i++) {
    pinMode(switch_pins[i], INPUT);
  }
}

void sensor_switches(void) {
  for (byte i = 0; i < NUM_SWITCHES; i++) {
    boolean value = (digitalRead(switch_pins[i]) == LOW);
    if (value != switch_states[i]) {
      switch_changed[i] = true;
      data_changed = true;
      switch_states[i] = value;
      if (value) {
	DEBUG_VALUELN(DEBUG_TRACE, "Switch is on: ", i);
      } else {
	DEBUG_VALUELN(DEBUG_TRACE, "Switch is off: ", i);
      }
    } else {
      switch_changed[i] = false;
    }
  }
}

/******* Capacitive Sensors ***************************************************/

void sensor_cap(void) 
{
  if (touch_sensor.readTouchInputs()) {
    DEBUG_COMMAND(DEBUG_TRACE,
      DEBUG_PRINT(DEBUG_TRACE, "Cap:");
      for (byte i = 0; i < MPR121::MAX_SENSORS; i++) {
        DEBUG_VALUE(DEBUG_TRACE, " ", touch_sensor.touched(i));
      }
      DEBUG_VALUELN(DEBUG_TRACE, " ms:", millis());
    );
    data_changed = true;
  }
}

/******* Handle Sensors *******************************************************/

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
  void ignite(boolean value);

  void enablePoof();
  void disablePoof();
  void poof(boolean value);
};

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
}

void Poofer::disableIgniter() {
  igniter_enabled = false;
}


void Poofer::enablePoof() {
  poof_enabled = true;
}

void Poofer::disablePoof() {
  poof_enabled = false;
}

void Poofer::ignite(boolean value) {
  if (value) {
    sendHMTLValue(address, igniter_output, 255);
  } else {
    sendHMTLValue(address, igniter_output, 0);
  }
}

void Poofer::poof(boolean value) {
  if (value) {
    sendHMTLValue(address, poof_output, 255);
  } else {
    sendHMTLValue(address, poof_output, 0);
  }
}

Poofer poof1(POOFER1_ADDRESS,
	     POOFER1_IGNITER_ENABLED, POOFER1_IGNITER,
	     POOFER1_POOF_ENABLED, POOFER1_POOF);
Poofer poof2(POOFER2_ADDRESS,
	     POOFER2_IGNITER_ENABLED, POOFER2_IGNITER,
	     POOFER2_POOF_ENABLED, POOFER2_POOF);

void handle_sensors(void) {
  static unsigned long last_send = millis();

  if (switch_changed[poof1.igniter_switch]) {
    if (switch_states[poof1.igniter_switch]) {
      poof1.enableIgniter();
    } else {
      poof1.disableIgniter();
    }
  }

  if (switch_changed[poof2.igniter_switch]) {
    if (switch_states[poof2.igniter_switch]) {
      poof2.enableIgniter();
    } else {
      poof2.disableIgniter();
    }
  }

  if (switch_changed[poof1.poof_switch]) {
    if (switch_states[poof1.poof_switch]) {
      poof1.enablePoof();
    } else {
      poof1.disablePoof();
    }
  }

  if (switch_changed[poof2.poof_switch]) {
    if (switch_states[poof2.poof_switch]) {
      poof2.enablePoof();
    } else {
      poof2.disablePoof();
    }
  }

  if (poof1.igniter_enabled && 
      touch_sensor.changed(POOFER1_IGNITER_SENSOR)) {
    poof1.ignite(touch_sensor.touched(POOFER1_IGNITER_SENSOR));
  }

  if (poof1.poof_enabled && 
      touch_sensor.changed(POOFER1_POOF_SENSOR)) {
    poof1.poof(touch_sensor.touched(POOFER1_POOF_SENSOR));
  }


  if (poof2.igniter_enabled && 
      touch_sensor.changed(POOFER2_IGNITER_SENSOR)) {
    poof2.ignite(touch_sensor.touched(POOFER2_IGNITER_SENSOR));
  }

  if (poof2.poof_enabled && 
      touch_sensor.changed(POOFER2_POOF_SENSOR)) {
    poof2.poof(touch_sensor.touched(POOFER2_POOF_SENSOR));
  }
}

void initialize_display() {
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("Initializing");
  lcd.setBacklight(HIGH);
}


void update_lcd() {
  if (data_changed) {
    //    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("C:");
    for (byte i = 0; i < MPR121::MAX_SENSORS; i++) {
      lcd.print(touch_sensor.touched(i));
    }

    lcd.setCursor(0, 1);
    lcd.print("S:");
    for (byte i = 0; i < NUM_SWITCHES; i++) {
      lcd.print(switch_states[i]);
    }
    
    data_changed = false;
  }
}
