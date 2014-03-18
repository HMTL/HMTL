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

#define HMTL_VERSION 1

#if HMTL_VERSION == 1
  #define RED_LED      9
  #define GREEN_LED   10
  #define BLUE_LED    11
  
  #define PIN_RS485_1  2
  #define PIN_RS485_2  7 
  #define PIN_RS485_3  4

  #define PIN_PIXEL_DATA 12
  #define PIN_PIXEL_CLOCK 8

  #define RCV_LED     13 // Only three LEDs on v2
#elif HMTL_VERSION == 2
  #define RED_LED     10
  #define GREEN_LED   11
  #define BLUE_LED    13

  #define PIN_RS485_1  4
  #define PIN_RS485_2  7 
  #define PIN_RS485_3  5
#endif

#define PIXELS
#ifdef PIXELS
  Adafruit_WS2801 strip = Adafruit_WS2801(105, PIN_PIXEL_DATA, PIN_PIXEL_CLOCK);
#else
  #define OUTPIN_1     8
  #define OUTPIN_2    12
#endif

#define MY_ADDR   0x01 // Socket receive address
 
RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

void setup() {
  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

#ifdef RCV_LED
  pinMode(RCV_LED, OUTPUT);
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

  DEBUG_VALUELN(DEBUG_LOW, "RS485 socket initialized. address=", MY_ADDR);
}


void loop() {
  unsigned int msglen;

  const byte *data = rs485.getMsg(MY_ADDR, &msglen);
  if (data != NULL) {
    int value = (data[0] << 8) | data[1];
    DEBUG_HEXVALLN(0, "value=", value);
#ifdef RCV_LED
    digitalWrite(RCV_LED, HIGH);
#endif
    if (value == 0) {
      DEBUG_PRINTLN(0, "Recieved reset");
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
#ifdef PIXELS
      setAllPixels(0, 0, 0);
#else
      digitalWrite(OUTPIN_1, LOW);
#endif
    } else {
      switch (value % 3) {
      case 0:
	digitalWrite(RED_LED, HIGH);
	digitalWrite(GREEN_LED, LOW);
	digitalWrite(BLUE_LED, LOW);
#ifdef PIXELS
	setAllPixels(255, 0, 0);
#else
	digitalWrite(OUTPIN_1, HIGH);
#endif
	break;
      case 1:
	digitalWrite(RED_LED, LOW);
	digitalWrite(GREEN_LED, HIGH);
	digitalWrite(BLUE_LED, LOW);
#ifdef PIXELS
	setAllPixels(0, 255, 0);
#endif
	break;
      case 2:
	digitalWrite(RED_LED, LOW);
	digitalWrite(GREEN_LED, LOW);
	digitalWrite(BLUE_LED, HIGH);
#ifdef PIXELS
	setAllPixels(0, 0, 255);
#endif
	break;
      }
    }
#ifdef PIXELS
    strip.show();
#endif
  } else {
    delay(20);
#ifdef RCV_LED
    digitalWrite(RCV_LED, LOW);
#endif
  }
}

#ifdef PIXELS
void setAllPixels(byte r, byte g, byte b) {
  for (unsigned int i=0; i < strip.numPixels(); i++) 
    strip.setPixelColor(i, r, g, b);
  strip.show();
}
#endif
