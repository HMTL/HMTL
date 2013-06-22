
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#include "RS485Utils.h"

RS485Socket rs485(2, 3, 4);

#define LED_PIN 13

void setup()
{
  Serial.begin(9600);
  pinMode (LED_PIN, OUTPUT);  // driver output enable

  rs485.setup();
}

byte i = 0;
boolean b = false;

void loop() {
  i++;

  rs485.sendMsgTo(0, &i, sizeof i);

  Serial.print("Sent ");
  Serial.println(i);

  b = !b;
  digitalWrite(LED_PIN, b);

  delay(500);
}
