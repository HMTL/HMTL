
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "RS485Utils.h"

RS485Socket rs485(2, 3, 4);

#define LED_PIN 13

byte databuffer[256];
byte *data;

void setup()
{
  Serial.begin(9600);
  pinMode (LED_PIN, OUTPUT);  // driver output enable

  rs485.setup();

  data = rs485.initBuffer(databuffer);
}

byte i = 0;
boolean b = false;
byte address = 0;

#define NUM_DEVICES 2

void loop() {
  i++;

  *(int *)data = i;

  rs485.sendMsgTo(address, data, sizeof i);

  b = !b;
  digitalWrite(LED_PIN, b);

  address = (address + 1) % NUM_DEVICES;
  delay(500);
}
