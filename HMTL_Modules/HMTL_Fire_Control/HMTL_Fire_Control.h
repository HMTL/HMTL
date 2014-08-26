/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 ******************************************************************************/

#ifndef HMTL_FIRE_CONTROL_H
#define HMTL_FIRE_CONTROL_H

#include "RS485Utils.h"
#include "MPR121.h"
#include "PixelUtil.h"
//#include <LiquidTWI.h>
#include "LiquidCrystal.h"


#define BAUD 9600

// LCD display
extern LiquidCrystal lcd;
void update_lcd();
void initialize_display();

// Pixels used for under lighting
extern PixelUtil pixels;

/***** Poofer bullshit ********************************************************/
void update_poofers();

/***** Sensor info ************************************************************/

/* All sensor info is recorded in a bit mask */
extern uint32_t sensor_state;

// XXX - Sensor macros go here?
#define SWITCH_PIN_1 5
#define SWITCH_PIN_2 6
#define SWITCH_PIN_3 9
#define SWITCH_PIN_4 10

void initialize_switches();
void sensor_switches();

extern MPR121 touch_sensor;
void sensor_cap();

void handle_sensors();

/***** Connectivity ***********************************************************/

extern RS485Socket rs485;
extern byte my_address;

#define ADDRESS_SOUND_UNIT  0x01

#define POOFER1_ADDRESS  0x41
#define POOFER1_IGNITER  0x0
#define POOFER1_POOF     0x1

#define POOFER2_ADDRESS  0x41
#define POOFER2_IGNITER  0x2
#define POOFER2_POOF     0x3

// Switches
#define POOFER1_IGNITER_ENABLED 0
#define POOFER1_POOF_ENABLED    2
#define POOFER2_IGNITER_ENABLED 1
#define POOFER2_POOF_ENABLED    3

// Sensors
#define POOFER1_QUICK_POOF_SENSOR   3
#define POOFER1_LONG_POOF_SENSOR    2
#define POOFER2_QUICK_POOF_SENSOR   0
#define POOFER2_LONG_POOF_SENSOR    1

#define SENSOR_EXTERNAL_1           4
#define SENSOR_EXTERNAL_2           5
#define SENSOR_EXTERNAL_3           6
#define SENSOR_EXTERNAL_4           7

#define SENSOR_LCD_LEFT             9
#define SENSOR_LCD_UP               8
#define SENSOR_LCD_DOWN            10
#define SENSOR_BOTTOM              11

void initialize_connect();

void sendHMTLValue(uint16_t address, uint8_t output, int value);
void sendHMTLTimedChange(uint16_t address, uint8_t output,
			 uint32_t change_period,
			 uint32_t start_color,
			 uint32_t stop_color);
#endif
