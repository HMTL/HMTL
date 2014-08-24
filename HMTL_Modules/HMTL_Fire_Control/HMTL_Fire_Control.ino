/*******************************************************************************
 * Code for the HMTL Fire control module:
 *   - LCD Display
 *   - 4 integral touch sensors, 4 optional external ones
 *   - 4 simple switches
 *
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 ******************************************************************************/

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "Wire.h"
//#include <LiquidTWI.h>
#include "LiquidCrystal.h"



#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

#include "HMTL_Fire_Control.h"

/* Auto update build number */
#define HMTL_FIRE_CONTROL_BUILD 4 // %META INCR

//LiquidTWI lcd(0);
LiquidCrystal lcd(0);

RS485Socket rs485;
PixelUtil pixels;
MPR121 touch_sensor; // MPR121 must be initialized after Wire.begin();
byte my_address = 0;

/******************************************************************************
 * Initialization
 *****************************************************************************/
void setup()
{
  Serial.begin(BAUD);
  DEBUG_PRINTLN(DEBUG_LOW, "*** HMTL Fire Control Initializing ***");
  DEBUG_VALUELN(DEBUG_LOW, "* Baud is ", BAUD);

  /* Initialize random see by reading from an unconnected analog pin */
  randomSeed(analogRead(0) + analogRead(2) + micros());

  /* Initialize display */
  initialize_display();

  lcd.setCursor(0, 0); lcd.print("hello, world!");
 
  readHMTLConfiguration();

  /* Setup the external connection */
  initialize_connect();

  /* Setup the sensors */
  initialize_switches();

  DEBUG_PRINT(DEBUG_LOW, "* Setup complete for Fire Control Module");
  DEBUG_VALUELN(DEBUG_LOW, " Build=", HMTL_FIRE_CONTROL_BUILD);
  DEBUG_MEMORY(DEBUG_HIGH);

  pixels.setAllRGB(255, 0, 255);
  pixels.update();
}

void loop()
{
  /* Check the sensor values */
  sensor_cap();

  sensor_switches();

  handle_sensors();

  // XXX: Do stuff here

  update_lcd();
}

void readHMTLConfiguration() {
  config_hdr_t config;
  output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
  config_max_t readoutputs[HMTL_MAX_OUTPUTS];
  int offset;

  uint32_t outputs_found = hmtl_setup(&config, readoutputs, outputs,
				     NULL, HMTL_MAX_OUTPUTS,
				     &rs485, &pixels, &touch_sensor,
				     NULL, NULL, &offset);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  if (!(outputs_found & (1 << HMTL_OUTPUT_PIXELS))) {
    DEBUG_ERR("No pixels config found");
    DEBUG_ERR_STATE(2);
  }

  if (!(outputs_found & (1 << HMTL_OUTPUT_MPR121))) {
    DEBUG_ERR("No mpr121 config found");
    DEBUG_ERR_STATE(3);
  }

  /* Store the configured address */
  my_address = config.address;
}
