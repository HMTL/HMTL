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
      data_changed = true;
      switch_states[i] = value;
      if (value) {
	DEBUG_VALUELN(DEBUG_TRACE, "Switch is on: ", i);
      } else {
	DEBUG_VALUELN(DEBUG_TRACE, "Switch is off: ", i);
      }
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

void handle_sensors(void) {
  static unsigned long last_send = millis();

  for (int i = 0; i < 4; i++) {
    if (touch_sensor.changed(i)) {
      if (touch_sensor.touched(i)) {
	sendHMTLValue(POOFER1_ADDRESS, i, 255);
      } else {
	sendHMTLValue(POOFER1_ADDRESS, i, 0);
      }
    }
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
