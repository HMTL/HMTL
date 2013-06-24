
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "RS485Utils.h"

RS485Socket rs485(2, 3, 4, false);

#define LED_PIN 13

void setup()
{
  Serial.begin(9600);
  pinMode (LED_PIN, OUTPUT);  // driver output enable

  rs485.setup();
}

byte i = 0;
boolean b = false;
byte address = 0;

void loop() {
  const byte *data = rs485.getMsg(address);
  if (data != NULL) {
    i = *(data);
    b = !b;
    digitalWrite(LED_PIN, b);
  }
}
