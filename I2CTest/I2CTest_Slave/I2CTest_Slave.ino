
#include <Wire.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define PACKET_OFF 'F'
#define PACKET_ON  'N'

void setup() {
  Wire.begin(0xF);
  Wire.onReceive(receiveEvent); // register event
  
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);
}

boolean led_on = false;

void loop() {
  if (led_on) digitalWrite(PIN_STATUS_LED, HIGH);
  else digitalWrite(PIN_STATUS_LED, LOW);
}

void receiveEvent(int howMany) {
  int val;
  for (int i = 0; i < howMany; i++) {
    val = Wire.read();    // receive byte as an integer
    if (i == 1) {
      if (val == PACKET_OFF) led_on = false;
      if (val == PACKET_ON) led_on = true;
    }
  }
}
