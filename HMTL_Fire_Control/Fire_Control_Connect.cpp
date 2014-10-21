/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * Code for communicating with remote modules
 ******************************************************************************/

#include <Arduino.h>
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Wire.h"
#include "Adafruit_WS2801.h"


#define DEBUG_LEVEL DEBUG_MID
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

#include "HMTL_Fire_Control.h"

#define SEND_BUFFER_SIZE RS485_BUFFER_TOTAL(sizeof (msg_hdr_t) + sizeof (msg_max_t) + 16) // XXX: Could this be smaller?

byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer; // Pointer to use for start of send data

void initialize_connect() {
  /* Setup the RS485 connection */
  if (!rs485.initialized) {
    DEBUG_ERR("RS485 was not initialized, check config");
    DEBUG_ERR_STATE(DEBUG_ERR_UNINIT);
  }

  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_VALUE(DEBUG_LOW, "Initialized RS485. address=", my_address);
  DEBUG_VALUELN(DEBUG_LOW, " bufsize=", SEND_BUFFER_SIZE);
}

void sendHMTLValue(uint16_t address, uint8_t output, int value) {
  hmtl_send_value(&rs485, send_buffer, SEND_BUFFER_SIZE,
		  address, output, value);
}

void sendHMTLTimedChange(uint16_t address, uint8_t output,
			 uint32_t change_period,
			 uint32_t start_color,
			 uint32_t stop_color) {
  hmtl_send_timed_change(&rs485, send_buffer, SEND_BUFFER_SIZE,
			 address, output,
			 change_period,
			 start_color,
			 stop_color);
}
