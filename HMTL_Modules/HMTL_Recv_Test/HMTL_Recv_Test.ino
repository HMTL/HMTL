/*
 * Basic RS485 socket receive test, triggers LEDs on receive
 */
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "Wire.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

/*
#define OUTPIN_1     8
#define OUTPIN_2    12
*/
 
RS485Socket rs485;
PixelUtil pixels;
config_rgb_t rgb_output;
config_value_t value_output = {{0, 0}, // hdr
			       -1,     // pin
			       0       // value
};

#define MAX_OUTPUTS 8
config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];
config_max_t readoutputs[MAX_OUTPUTS];

void setup() {
  Serial.begin(9600);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL_Recv_Test initializing");

  int configOffset = hmtl_read_config(&config, 
				      readoutputs, 
				      MAX_OUTPUTS);
  if (configOffset < 0) {
    DEBUG_ERR("Failed to read configuration");
    DEBUG_ERR_STATE(12);
  }

  DEBUG_VALUELN(DEBUG_LOW, "Read config.  offset=", configOffset);
  for (int i = 0; i < config.num_outputs; i++) {
    if (i >= MAX_OUTPUTS) {
      DEBUG_VALUELN(0, "Too many outputs:", config.num_outputs);
      return;
    }
    outputs[i] = (output_hdr_t *)&readoutputs[i];
  }

  DEBUG_COMMAND(DEBUG_HIGH, hmtl_print_config(&config, outputs));

  /* Initialize the outputs */
  boolean has_rgb = false;
  boolean has_pixels = false;
  boolean has_rs485 = false;
  boolean has_value = false;
  for (int i = 0; i < config.num_outputs; i++) {
    void *data = NULL;
    switch (((output_hdr_t *)outputs[i])->type) {
    case HMTL_OUTPUT_PIXELS: data = &pixels; has_pixels = true; break;
    case HMTL_OUTPUT_RS485: data = &rs485; has_rs485 = true; break;
    case HMTL_OUTPUT_RGB: {
      memcpy(&rgb_output, outputs[i], sizeof (rgb_output));
      has_rgb = true;
    }
    case HMTL_OUTPUT_VALUE: {
      memcpy(&value_output, outputs[i], sizeof (value_output));
      has_value = true;
    }
    }
    hmtl_setup_output((output_hdr_t *)outputs[i], data);
  }

  if (!has_rgb || !has_pixels || !has_rs485) {
    DEBUG_ERR("Does not have all required configs!");
    DEBUG_ERR_STATE(13);
  }

  rs485.setup();

  for (unsigned int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelRGB(i, 0, 0, 0);
  }
  pixels.update();

  DEBUG_VALUELN(DEBUG_LOW, "RS485 socket initialized. device_id=", config.address);
}

unsigned long lastRecv = 0;
void loop() {
  unsigned int msglen;

  const byte *data = rs485.getMsg(config.address, &msglen);
  //const byte *data = rs485.getMsg(RS485_ADDR_ANY, &msglen);
  if (data != NULL) {
    unsigned long now = millis();
    DEBUG_VALUE(0, "Recv len=", msglen);
    DEBUG_VALUE(0, " addr=", RS485_HDR_FROM_DATA(data)->address);
    DEBUG_VALUE(0, " delay=", now - lastRecv);

    lastRecv = millis();
    
    if (msglen == 4) {
      static unsigned long lastChanged = 0;
      static uint32_t lastValue = 0;
      static uint32_t fadeCount = 0;

      uint32_t value = ((uint32_t)data[0] << 24) |
	((uint32_t)data[1] << 16) |
	((uint32_t)data[2] << 8) | 
	(uint32_t)data[3];
      DEBUG_HEXVAL(0, " data=", data[0]);
      DEBUG_HEXVAL(0, ".", data[1]);
      DEBUG_HEXVAL(0, ".", data[2]);
      DEBUG_HEXVAL(0, ".", data[3]);
      
      DEBUG_HEXVAL(0, " rgb=", pixel_red(value));
      DEBUG_HEXVAL(0, ",", pixel_green(value));
      DEBUG_HEXVAL(0, ",", pixel_blue(value));

      DEBUG_PRINT_END();

      if (value != lastValue) {
	lastChanged = millis();
	setAllPixels(pixel_red(value), pixel_green(value), pixel_blue(value));
	lastValue = value;
	fadeCount = 0;
      } else {

	if ((unsigned long)(millis() - lastChanged) >
			    (unsigned long)(60000)) {
	  // Turn off if nothing changed for a minute
	  setAllPixels(0, 0, 0);
	}
      }
    } else {
      int value = (data[0] << 8) | data[1];
      DEBUG_HEXVALLN(0, " value=", value);
      if (value_output.pin != (byte)-1) digitalWrite(value_output.pin, HIGH);
      if (value == 0) {
	DEBUG_PRINTLN(0, "Recieved reset");
	digitalWrite(rgb_output.pins[0], LOW);
	digitalWrite(rgb_output.pins[1], LOW);
	digitalWrite(rgb_output.pins[2], LOW);
	setAllPixels(0, 0, 0);
      } else {
	switch (value % 3) {
	case 0:
	  digitalWrite(rgb_output.pins[0], HIGH);
	  digitalWrite(rgb_output.pins[1], LOW);
	  digitalWrite(rgb_output.pins[2], LOW);
	  setAllPixels(255, 0, 0);
	  break;
	case 1:
	  digitalWrite(rgb_output.pins[0], LOW);
	  digitalWrite(rgb_output.pins[1], HIGH);
	  digitalWrite(rgb_output.pins[2], LOW);
	  setAllPixels(0, 255, 0);
	  break;
	case 2:
	  digitalWrite(rgb_output.pins[0], LOW);
	  digitalWrite(rgb_output.pins[1], LOW);
	  digitalWrite(rgb_output.pins[2], HIGH);
	  setAllPixels(0, 0, 255);
	  break;
	}
      }
    }
  } else {
    delay(20);
    if (value_output.pin != (byte)-1) digitalWrite(value_output.pin, LOW);
  }

  if ((millis() - lastRecv) > (unsigned long)1000) {
    // Turn off if we stop receiving
    setAllPixels(0, 0, 0);
  }

  pixels.update();
}

void setAllPixels(byte r, byte g, byte b) {
  for (unsigned int i=0; i < pixels.numPixels(); i++) 
    pixels.setPixelRGB(i, r, g, b);
}
