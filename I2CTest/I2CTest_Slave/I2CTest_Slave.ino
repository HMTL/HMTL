
#include <Wire.h>
#include <EEPROM.h>

#include <I2CUtils.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define PACKET_OFF 'F'
#define PACKET_ON  'N'

int my_address = 5;

void setup() {
  Serial.begin(9600);

  // Get the address from EEProm
  int read_address = I2C_read_or_write_address(my_address);
  if (read_address != -1) {
    my_address = read_address;
  }
  
  Wire.begin(my_address);
  Wire.onReceive(receiveEvent); // register event
  
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);

  for (int pin = 0; pin <= 13; pin++) {    
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

boolean pin_on[14] = {
  false, false, false, false,
  false, false, false, false,
  false, false, false, false,
  false, false
};


int lastUpdate = 0;
void loop() {
  for (int pin = 0; pin <= 13; pin++) {
    if (pin_on[pin]) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
  }
  
  //Serial.print(my_address);

  blink_value(PIN_DEBUG_LED, my_address, 500, 4);

  delay(10);
}

void receiveEvent(int howMany) {
  int val;
  int pin;

  for (int i = 0; i < howMany; i++) {
    val = Wire.read();    // receive byte as an integer
    if (i % 2 == 0) {
      pin = val;
    }
    if (i % 2 == 1) {
      if (val == PACKET_OFF) pin_on[pin] = false;
      if (val == PACKET_ON) pin_on[pin] = true;
    }
  }
}
