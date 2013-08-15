
#include <Wire.h>
#include <EEPROM.h>

#include <I2CUtils.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

int my_address = 0;

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

#define NUM_PINS 14
byte pin_value[NUM_PINS] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0
};


int lastUpdate = 0;
void loop() {
  for (int pin = 0; pin <= 13; pin++) {
    if (pin_is_PWM(pin)) {
      if (pin_value[pin] == 0) digitalWrite(pin, LOW);
      else if (pin_value[pin] == 255) digitalWrite(pin, HIGH);
      else analogWrite(pin, pin_value[pin]);
    } else {
      if (pin_value[pin] > 0) digitalWrite(pin, HIGH);
      else digitalWrite(pin, LOW);
    }
  }
  
  //Serial.print(my_address);

  blink_value(PIN_DEBUG_LED, my_address, 500, 4);

  delay(100);
}

void receiveEvent(int msg_len) {
  message_t msg;
  int read = 0;

  while (read < msg_len) {
    I2C_read_struct((byte *)&msg, sizeof (msg));
    pin_value[msg.pin] = msg.value;
    read += sizeof (msg);
  }
}
