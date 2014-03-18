/*
 * Basic RS485 socket send test
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

#define HMTL_VERSION 3

#if HMTL_VERSION == 1
  #define RED_LED      9
  #define GREEN_LED   10
  #define BLUE_LED    11
  #define WHITE_LED     13 // Only three LEDs on v2

  #define PIN_RS485_1  2
  #define PIN_RS485_2  7 
  #define PIN_RS485_3  4

  #define PIN_PIXEL_DATA 12
  #define PIN_PIXEL_CLOCK 8
#elif (HMTL_VERSION == 2) || (HMTL_VERSION == 3)
  #define RED_LED     10
  #define GREEN_LED   11
  #define BLUE_LED    13

  #define PIN_RS485_1  4
  #define PIN_RS485_2  7 
  #define PIN_RS485_3  5

  #define PIN_PIXEL_DATA 12
  #define PIN_PIXEL_CLOCK 8
#endif

#define PIXELS
#ifdef PIXELS
  Adafruit_WS2801 strip = Adafruit_WS2801(105, PIN_PIXEL_DATA, PIN_PIXEL_CLOCK);
#else
  #define OUTPIN_1     8
  #define OUTPIN_2    12
#endif

#define DEST_ADDR   0x01 // Socket receive address
 
RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));
#define SEND_BUFFER_SIZE (sizeof (rs485_socket_hdr_t) + 64)
byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup() {
  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

#ifdef WHITE_LED
  pinMode(WHITE_LED, OUTPUT);
#endif

#ifdef PIXELS
  strip.begin();
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
#else
  pinMode(OUTPIN_1, OUTPUT);
#endif

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL Send test initialized");
}

int cycle = 0;

void loop() {

  // Send the current cycle to the remote
  send_buffer[0] = (cycle & 0xFF00) >> 8;
  send_buffer[1] = (cycle & 0x00FF);
  rs485.sendMsgTo(DEST_ADDR, send_buffer, 2);

  switch (cycle % 4) {
  case 0:
    {
      DEBUG_PRINTLN(0, "Recieved reset");
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
#ifdef WHITE_LED
      digitalWrite(WHITE_LED, LOW);
#endif
#ifdef PIXELS
      for (unsigned int i=0; i < strip.numPixels(); i++) 
	strip.setPixelColor(i, 255, 255, 255);
#else
      digitalWrite(OUTPIN_1, LOW);
#endif
      break;
    }
  case 1: {
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
#ifdef PIXELS
    for (unsigned int i=0; i < strip.numPixels(); i++) 
      strip.setPixelColor(i, 255, 0, 0);
#else
    digitalWrite(OUTPIN_1, HIGH);
#endif
    break;
    case 2: {
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(BLUE_LED, LOW);
#ifdef PIXELS
      for (unsigned int i=0; i < strip.numPixels(); i++) 
	strip.setPixelColor(i, 0, 255, 0);
#endif
      break;
    }
    case 3: {
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, HIGH);
#ifdef PIXELS
      for (unsigned int i=0; i < strip.numPixels(); i++) 
	strip.setPixelColor(i, 0, 0, 255);
#endif
      break;
    }
  }
  }

  delay(1000);
  cycle++;
}
