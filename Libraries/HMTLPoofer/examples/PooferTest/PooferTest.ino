/*******************************************************************************
 * A test sketch for communicating the HMTL Poofers.
 *
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 ******************************************************************************/

#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

// Arduino IDE required BS
#include "EEPROM.h"
#include "FastLED.h"
#include "Wire.h"
#include "MPR121.h"
#include "XBee.h"
#include "XBeeSocket.h"

// HMTL required BS
#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "PixelUtil.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "Socket.h"
#include "RS485Utils.h"
//#include "MPR121.h"

#include "HMTLPoofer.h"

#define DATA_SIZE (sizeof (msg_hdr_t) + sizeof (msg_max_t) + 16)
#define SEND_BUFFER_SIZE RS485_BUFFER_TOTAL(DATA_SIZE)
byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer; // Pointer to use for start of send data

RS485Socket rs485(2, 7, 4, 0);

#define POOFER_ADDR 2
#define IGNITION_OUTPUT 0
#define PILOT_OUTPUT 1
#define POOF_OUTPUT 2
Poofer *poofer;

#define STATUS_LED 13


void setup() {
  Serial.begin(9600);

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);

  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer, DATA_SIZE);

  poofer = new Poofer(1, POOFER_ADDR, 
                      IGNITION_OUTPUT, PILOT_OUTPUT, POOF_OUTPUT,  
                      &rs485, send_buffer, DATA_SIZE);
}

#define POOFER_OFF 0
#define IGNITING   1
#define POOF_OK    2
#define POOFING    3

byte state = POOFER_OFF;

uint32_t last_poof_ms = 0;
#define POOF_LENGTH 1000
#define POOF_PERIOD 10000

void loop() {
  switch (state) {
    case POOFER_OFF: {
      // Start the ignition cycle
      DEBUG1_PRINTLN("* Starting ignition cycle");
      poofer->enableIgniter();
      state = IGNITING;
      break;
    }

    case IGNITING: {
      // Wait for the ignition sequence to be complete
      if (poofer->checkState(Poofer::POOF_READY)) {
        DEBUG1_PRINTLN("* Ignition cycle complete");
        state = POOF_OK;
        poofer->enablePoof();
      } else {
        DEBUG1_VALUELN("* Remaining: ", poofer->ignite_remaining());
        delay(1000);
      }
      break;
    }

    case POOF_OK: {
      // Periodically trigger a poof
      if (millis() - last_poof_ms >= POOF_PERIOD) {
        DEBUG1_PRINTLN("* Initiating poof");
        last_poof_ms = millis();
        poofer->poof(POOF_LENGTH);
        state = POOFING;
        digitalWrite(STATUS_LED, HIGH);
      }
      break;
    }

    case POOFING: {
      // Wait for the poof to complete
      if (!poofer->checkState(Poofer::POOF_ON)) {
        DEBUG1_PRINTLN("* Poof completed");
        state = POOF_OK;
        digitalWrite(STATUS_LED, LOW);
      }
      break;
    }
  }

  // Allow the poofer to update its state
  poofer->update();
}
