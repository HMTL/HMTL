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

#define PIN_RS485_1  2 // 2 on board v1, 4 on board v2
#define PIN_RS485_2  7 
#define PIN_RS485_3  4 // 4 on board v1, 5 on board v2

#define RED_LED      9 // 10 on board v2
#define GREEN_LED   10 // 11 on board v2
#define BLUE_LED    11 // 13 on board v2

#define RCV_LED     13 // Only three LEDs on v2

#define MY_ADDR   0x01 // Socket receive address

 
RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

void setup() {
  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RCV_LED, OUTPUT);

  /* Setup the RS485 connection */  
  rs485.setup();

  DEBUG_PRINTLN(DEBUG_LOW, "RS485 socket initialized");
}

void loop() {
  unsigned int msglen;

  const byte *data = rs485.getMsg(MY_ADDR, &msglen);
  if (data != NULL) {
    int value = (data[0] << 8) | data[1];
    DEBUG_VALUELN(0, "value=", value);
    digitalWrite(RCV_LED, HIGH);
    if (value == 0) {
      DEBUG_PRINTLN(0, "Recieved reset");
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
    } else {
      switch (value % 3) {
      case 0:
	digitalWrite(RED_LED, HIGH);
	digitalWrite(GREEN_LED, LOW);
	digitalWrite(BLUE_LED, LOW);
	break;
      case 1:
	digitalWrite(RED_LED, LOW);
	digitalWrite(GREEN_LED, HIGH);
	digitalWrite(BLUE_LED, LOW);
	break;
      case 2:
	digitalWrite(RED_LED, LOW);
	digitalWrite(GREEN_LED, LOW);
	digitalWrite(BLUE_LED, HIGH);
	break;
      }
    }
  } else {
    delay(20);
    digitalWrite(RCV_LED, LOW);
  }
}
