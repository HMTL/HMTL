
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h> // XXX
#include <Wire.h> // XXX


#include "I2CUtils.h" // XXX
#include "RS485Utils.h"

RS485Socket rs485(2, 3, 4, false);

#define PIN_DEBUG_LED 13

int my_address = 1;

#define NUM_PINS 3
byte pin_num[NUM_PINS] = {
  10, 11, 12
};
byte pin_value[NUM_PINS] = {
  0, 0, 0,
};


void setup()
{
  Serial.begin(9600);

  for (int pin_index = 0; pin_index < NUM_PINS; pin_index++) {
    int pin = pin_num[pin_index];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  pinMode(PIN_DEBUG_LED, OUTPUT);  // driver output enable

  rs485.setup();
}


boolean b = false;

boolean update;

void loop() {
  update = false;

  /* Check for messages to this address */
  read_state();

  /* If state changed then updated all pins */
  if (update) {
    for (int pin_index = 0; pin_index < NUM_PINS; pin_index++) {
      int pin = pin_num[pin_index];
      if (pin_is_PWM(pin)) {
        if (pin_value[pin] == 0) digitalWrite(pin, LOW);
        else if (pin_value[pin] == 255) digitalWrite(pin, HIGH);
        else analogWrite(pin, pin_value[pin]);
      } else {
        if (pin_value[pin] > 0) digitalWrite(pin, HIGH);
        else digitalWrite(pin, LOW);
      }
    }
  }

  blink_value(PIN_DEBUG_LED, my_address, 500, 4);

  //delay(100);
}

void read_state() 
{
  unsigned int msglen;

  const byte *data = rs485.getMsg(my_address, &msglen);

  if (data != NULL) {
    if (msglen < sizeof (message_t)) {
      Serial.println("ERROR: msglen less than message size");
      return;
    }

    message_t *msg = (message_t *)data;
    unsigned int processed = 0;

    Serial.print("read_state:");
    Serial.print(msglen);
    do {
      if (processed > (msglen - sizeof (message_t))) {
        Serial.print("ERROR: read_state: msg_len wasn't multiple of message_t");
        break;
      }

      Serial.print(" ");
      Serial.print(msg->pin, HEX);
      Serial.print(":");
      Serial.print(msg->value, HEX);

      if (msg->pin > NUM_PINS) {
        Serial.print("*");
      } else {
        pin_value[pin_num[msg->pin]] = msg->value;
      }

      processed += sizeof (message_t);
      msg = msg + 1;
    } while (processed < msglen);
    Serial.println();

    update = true;
  }
}
