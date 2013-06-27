
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include <EEPROM.h> // XXX
#include <Wire.h> // XXX

#include "I2CUtils.h" // XXX
#include "RS485Utils.h"

RS485Socket rs485(2, 3, 4, true);

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define NUM_SLAVES 6
#define NUM_PINS 3
boolean led_value[NUM_SLAVES][NUM_PINS] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};
int slave_id[NUM_SLAVES] = {0, 1, 2, 3, 4, 5};
int slave_pin[NUM_SLAVES][NUM_PINS] = {
  {9, 10, 11},
  {9, 10, 11},
  {9, 10, 11},
  {9, 10, 11},
  {9, 10, 11},
  {9, 10, 11},
};

#define MAX_MSG_DATA     (NUM_PINS * sizeof(message_t))
#define SEND_BUFFER_SIZE (MAX_MSG_DATA + sizeof (rs485_socket_hdr_t) + 64)

byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, HIGH);

  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);
}


boolean debug_led = false;

int cycle = 0;

#define MODE   3
#define PERIOD 100

void loop()
{
  long start = millis();
  int slave;

  update_state();

  for (slave = 0; slave < NUM_SLAVES; slave++) {
    send_state(slave);
  }

  debug_led = !debug_led;
  if (debug_led) digitalWrite(PIN_DEBUG_LED, HIGH);
  else digitalWrite(PIN_DEBUG_LED, LOW);

  int elapsed = millis() - start;
  delay(PERIOD <= elapsed ? 0 : PERIOD - elapsed);
  cycle++;
}

/* Update hte state of all devices */
void update_state() 
{
  int slave;
  int pin;

  switch (MODE) {
      case 0:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = (led_value[slave][0] == 0 ? 255 : 0);
        }
        break;
      case 1:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 2:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
          led_value[slave][1] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 3:
        static int current_pin = 0;
        static int current_slave = 0;
        static boolean current_on = true;

        led_value[current_slave][current_pin] = (current_on ? 255 : 0);

        if (!current_on) {
          current_pin = (current_pin + 1) % NUM_PINS;
          if (current_pin == 0) current_slave = (current_slave + 1) % NUM_SLAVES;
        }
        current_on = !current_on;
        break;
      case 4:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          for (pin = 0; pin < NUM_PINS; pin++) {
            led_value[slave][pin] = (led_value[slave][pin] + 10) % 256;
          }
        }

        break;
  }
}

/* Send a state update to the indicated device */
void send_state(int slave) 
{
  /* Fill in the send buffer with this address's pin state */
  message_t *msg;
  unsigned int msg_len = 0;
  for (int pin = 0; pin < NUM_PINS; pin++) {
    if (msg_len > (MAX_MSG_DATA - sizeof (message_t))) {
      /* Prevent buffer overflow */
      Serial.println("ERROR: send_state: msg exceeded length");
      break;
    }

    msg = (message_t *)(&send_buffer[0] + pin * sizeof (message_t));
    msg_len += sizeof (message_t);

    msg->pin = slave_pin[slave][pin];
    msg->value = led_value[slave][pin];
  }

  rs485.sendMsgTo(slave_id[slave], send_buffer, msg_len);
}
