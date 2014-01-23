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

#define PIN_RS485_1     2 // 4 on board v2
#define PIN_RS485_2     7 
#define PIN_RS485_3     4 // 5 on board v2

#define RCV_LED 13
 
RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

void setup() {
  Serial.begin(9600);

  pinMode(RCV_LED, OUTPUT);

  /* Setup the RS485 connection */  
  rs485.setup();
}

#define MY_ADDR 1
void loop() {
  unsigned int msglen;

  const byte *data = rs485.getMsg(MY_ADDR, &msglen);
  if (data != NULL) {
    char letter = data[0];
    byte count = data[1];
    DEBUG_VALUE(0, "letter=", letter);
    DEBUG_VALUELN(0, " count=", count);
    digitalWrite(RCV_LED, HIGH);
  } else {
    delay(10);
    digitalWrite(RCV_LED, LOW);
  }
}
