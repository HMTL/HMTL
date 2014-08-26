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
#include "Poofer.h"

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

Poofer poof1(POOFER1_ADDRESS,
	     POOFER1_IGNITER_ENABLED, POOFER1_IGNITER,
	     POOFER1_POOF_ENABLED, POOFER1_POOF);
Poofer poof2(POOFER2_ADDRESS,
	     POOFER2_IGNITER_ENABLED, POOFER2_IGNITER,
	     POOFER2_POOF_ENABLED, POOFER2_POOF);

byte display_mode = 0;
#define NUM_DISPLAY_MODES 2

void handle_sensors(void) {
  static unsigned long last_send = millis();

  /* Igniter switches */
  if (switch_changed[poof1.igniter_switch]) {
    if (switch_states[poof1.igniter_switch]) {
      poof1.enableIgniter();
    } else {
      poof1.disableIgniter();
    }
  }

  if (switch_changed[poof1.poof_switch]) {
    if (switch_states[poof1.poof_switch]) {
      poof1.enablePoof();
    } else {
      poof1.disablePoof();
    }
  }

  /* Poof switches */
  if (switch_changed[poof2.igniter_switch]) {
    if (switch_states[poof2.igniter_switch]) {
      poof2.enableIgniter();
    } else {
      poof2.disableIgniter();
    }
  }

  if (switch_changed[poof2.poof_switch]) {
    if (switch_states[poof2.poof_switch]) {
      poof2.enablePoof();
    } else {
      poof2.disablePoof();
    }
  }

  /* Poofer controls */
  if (poof1.igniter_enabled && poof1.poof_enabled && 
      touch_sensor.changed(POOFER1_QUICK_POOF_SENSOR) &&
      touch_sensor.touched(POOFER1_QUICK_POOF_SENSOR)) {
    poof1.poof(50);
  }

  if (poof1.igniter_enabled && poof1.poof_enabled && 
      touch_sensor.touched(POOFER1_LONG_POOF_SENSOR)) {
    poof1.poof(100);
  }

  if (poof2.igniter_enabled && poof2.poof_enabled && 
      touch_sensor.changed(POOFER2_QUICK_POOF_SENSOR) &&
      touch_sensor.touched(POOFER2_QUICK_POOF_SENSOR)) {
    poof2.poof(50);
  }

  if (poof2.igniter_enabled && poof2.poof_enabled && 
      touch_sensor.touched(POOFER2_LONG_POOF_SENSOR)) {
    poof2.poof(100);
  }

  /* Change display mode */
  if (touch_sensor.changed(SENSOR_LCD_LEFT) &&
      touch_sensor.touched(SENSOR_LCD_LEFT)) {
    lcd.clear();
    display_mode = (display_mode + 1) % NUM_DISPLAY_MODES;
  }
}

void initialize_display() {
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("Initializing");
  lcd.setBacklight(HIGH);
}


void update_lcd() {
  switch (display_mode) {
  case 0: {
    if (data_changed) {
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
    break;
  }
  case 1: {
    lcd.setCursor(0, 0);
    lcd.print("FIRE 1:");
    if (poof1.igniter_enabled) {
      lcd.print(" I");
    } else {
      lcd.print(" X");
    }

    if (poof1.poof_enabled) {
      lcd.print(" P");
    } else {
      lcd.print(" X");
    }

    lcd.setCursor(0, 1);
    lcd.print("FIRE 2:");
    if (poof2.igniter_enabled) {
      lcd.print(" I");
    } else {
      lcd.print(" X");
    }

    if (poof2.poof_enabled) {
      lcd.print(" P");
    } else {
      lcd.print(" X");
    }
    break;
  }
  }
}
