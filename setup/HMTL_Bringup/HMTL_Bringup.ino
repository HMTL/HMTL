/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * Bringup code for generic HMTL boards
 */

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "SPI.h"
#include "FastLED.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "Wire.h"
#include "MPR121.h"
#include "SerialCLI.h"

#include "Socket.h"
#include "RS485Utils.h"
#include "XBee.h"
#include "XBeeSocket.h"

#include "HMTLProtocol.h"
#include "HMTLMessaging.h"
#include "HMTLPrograms.h"

/******/

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_hdr_t readconfig;
config_max_t readoutputs[HMTL_MAX_OUTPUTS];

config_rgb_t rgb_output;
config_pixels_t pixel_output;
config_value_t value_output;
config_rs485_t rs485_output;

boolean has_value = false;
boolean has_pixels = false;
boolean has_rs485 = false;

int configOffset = -1;

PixelUtil pixels;

RS485Socket rs485;
#define SEND_BUFFER_SIZE 64 // The data size for transmission buffers
byte rs485_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

void setup() {
  Serial.begin(9600);

  DEBUG2_PRINTLN("***** HMTL Bringup *****");

  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     outputs, NULL, HMTL_MAX_OUTPUTS,
                                     &rs485, 
                                     NULL,
                                     &pixels, 
                                     NULL, // MPR121
                                     &rgb_output, // RGB
                                     &value_output, // Value
                                     &configOffset);

  DEBUG4_VALUE("Config size:", configOffset - HMTL_CONFIG_ADDR);
  DEBUG4_VALUELN(" end:", configOffset);
  DEBUG4_COMMAND(hmtl_print_config(&config, outputs));

  /* Setup the RS485 connection if one is configured */
  if (outputs_found & (1 << HMTL_OUTPUT_RS485)) {
    rs485.setup();
    rs485.initBuffer(rs485_data_buffer, SEND_BUFFER_SIZE);
    has_rs485 = true;
  }

  if (outputs_found & (1 << HMTL_OUTPUT_VALUE)) {
    has_value = true;
  }

  if (outputs_found & (1 << HMTL_OUTPUT_PIXELS)) {
    for (unsigned int i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelRGB(i, 0, 0, 0);
    }
    pixels.update();
    has_pixels = true;
  }
}

#define PERIOD 1000
unsigned long last_change = 0;
int cycle = 0;

void loop() {
  unsigned long now = millis();
  
  /*
   * Change the display mode periodically
   */
  if (now - last_change > PERIOD) {

    // Set LED colors
    switch (cycle % 4) {
      case 0: {
        DEBUG1_PRINTLN("White");
        if (has_value) digitalWrite(value_output.pin, HIGH);
        if (has_pixels) {
          for (unsigned int i=0; i < pixels.numPixels(); i++) 
            pixels.setPixelRGB(i, 255, 255, 255);  
          pixels.update();
        }
        break;
      }

      case 1: {
        DEBUG1_PRINTLN("Red");
        if (has_value) digitalWrite(value_output.pin, LOW);
        digitalWrite(rgb_output.pins[0], HIGH);
        if (has_pixels) {
          for (unsigned int i=0; i < pixels.numPixels(); i++) 
            pixels.setPixelRGB(i, 255, 0, 0);  
          pixels.update();
        }
        break;
      }

      case 2: {
        DEBUG1_PRINTLN("Green");
        digitalWrite(rgb_output.pins[0], LOW);
        digitalWrite(rgb_output.pins[1], HIGH);
        if (has_pixels) {
          for (unsigned int i=0; i < pixels.numPixels(); i++) 
            pixels.setPixelRGB(i, 0, 255, 0);  
          pixels.update();
        }
        break;
      }

      case 3: {
        DEBUG1_PRINTLN("Blue");
        digitalWrite(rgb_output.pins[1], LOW);
        digitalWrite(rgb_output.pins[2], HIGH);
        if (has_pixels) {
          for (unsigned int i=0; i < pixels.numPixels(); i++) 
            pixels.setPixelRGB(i, 0, 0, 255);  
          pixels.update();
        }

        if (has_rs485) {
          // Broadcast a message
          DEBUG1_PRINTLN("Sending rs485");
          hmtl_send_cancel(&rs485, rs485.send_buffer, rs485.send_data_size, 
                           SOCKET_ADDR_ANY, 0);
        }

        break;
      }
    }


    cycle++;
    last_change = now;
  }

  if (has_rs485) {
    /*
     * Check for data over RS485
     */
    unsigned int msglen;
    msg_hdr_t *msg = hmtl_rs485_getmsg(&rs485, &msglen, config.address);
    if (msg != NULL) {
      DEBUG1_VALUE("Recieved rs485 msg len:", msglen);
      DEBUG1_VALUE(" src:", RS485_SOURCE_FROM_DATA(msg));
      DEBUG1_VALUE(" dst:", RS485_ADDRESS_FROM_DATA(msg));
      DEBUG1_VALUE(" len:", msg->length);
      DEBUG1_VALUE(" type:", msg->type);
      DEBUG1_HEXVAL(" flags:0x", msg->flags);
      DEBUG1_PRINT(" data:");
      DEBUG1_COMMAND(
                     print_hex_string((byte *)msg, msglen)
                     );
      DEBUG_PRINT_END();
      
      // TODO: Move code from command cli into library
    }

  }
}
