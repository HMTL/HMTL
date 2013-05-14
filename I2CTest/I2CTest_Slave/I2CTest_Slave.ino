
#include <Wire.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define PIN_ADDRESS_BIT0 2
#define PIN_ADDRESS_BIT1 3

#define PACKET_OFF 'F'
#define PACKET_ON  'N'

int my_address = 0;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ADDRESS_BIT0, INPUT_PULLUP);
  pinMode(PIN_ADDRESS_BIT1, INPUT_PULLUP);
  if (my_address == 0) {
    if (digitalRead(PIN_ADDRESS_BIT0) == HIGH) my_address |= 0x1;
    if (digitalRead(PIN_ADDRESS_BIT1) == HIGH) my_address |= 0x2;
  }
  
  Wire.begin(my_address);
  Wire.onReceive(receiveEvent); // register event
  
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);
}

boolean led_on = false;
int lastUpdate = 0;
void loop() {
  if (led_on) digitalWrite(PIN_STATUS_LED, HIGH);
  else digitalWrite(PIN_STATUS_LED, LOW);

  Serial.print(my_address);
  Serial.print(" ");
  Serial.print(digitalRead(PIN_ADDRESS_BIT0));
  Serial.print(" ");
  Serial.println(digitalRead(PIN_ADDRESS_BIT1));
  
  delay(10);
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
