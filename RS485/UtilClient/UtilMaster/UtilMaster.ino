
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "RS485Utils.h"


/* Pin definitions */
#define PIN_LIGHT_SENSE A0

#define PIN_RS485_1     2
#define PIN_RS485_2     7 // XXX: This changed from 3 on the old ones
#define PIN_RS485_3     4

#define PIN_OUTPUT1     9
#define PIN_RCVD_LED   10
#define PIN_XMIT_LED   11
#define PIN_POWER_LED  12
#define PIN_DEBUG_LED  13

RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

#define NUM_SLAVES 3
#define NUM_OUTPUTS 3
boolean output_value[NUM_SLAVES][NUM_OUTPUTS] = {
  {0, 0, 0},
#if NUM_SLAVES > 1
  {0, 0, 0},
#endif
#if NUM_SLAVES > 2
  {0, 0, 0},
#endif
#if NUM_SLAVES > 3
  {0, 0, 0},
#endif
#if NUM_SLAVES > 4
  {0, 0, 0},
#endif
#if NUM_SLAVES > 5
  {0, 0, 0},
#endif
};
int slave_id[NUM_SLAVES] = {
  0,
#if NUM_SLAVES > 1
  1,
#endif
#if NUM_SLAVES > 2
  2,
#endif
#if NUM_SLAVES > 3
  3,
#endif
#if NUM_SLAVES > 4
  4,
#endif
#if NUM_SLAVES > 5
  5
#endif
};
int slave_output[NUM_SLAVES][NUM_OUTPUTS] = {
  {0, 1, 2},
#if NUM_SLAVES > 1
  {0, 1, 2},
#endif
#if NUM_SLAVES > 2
  {0, 1, 2},
#endif
#if NUM_SLAVES > 3
  {0, 1, 2},
#endif
#if NUM_SLAVES > 4
  {0, 1, 2},
#endif
#if NUM_SLAVES > 5
  {0, 1, 2},
#endif
};

#define MAX_MSG_DATA     (NUM_OUTPUTS * sizeof(msg_output_value_t))
#define SEND_BUFFER_SIZE (MAX_MSG_DATA + sizeof (rs485_socket_hdr_t) + 64)

byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup()
{
  Serial.begin(9600);

  pinMode(PIN_OUTPUT1, OUTPUT);
  digitalWrite(PIN_OUTPUT1, LOW);

  pinMode(PIN_RCVD_LED, OUTPUT);
  digitalWrite(PIN_RCVD_LED, LOW);

  pinMode(PIN_XMIT_LED, OUTPUT);
  digitalWrite(PIN_XMIT_LED, LOW);

  pinMode(PIN_POWER_LED, OUTPUT);
  digitalWrite(PIN_POWER_LED, HIGH);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, HIGH);

  pinMode(PIN_LIGHT_SENSE, INPUT_PULLUP);

  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);
}


boolean debug_led = false;
boolean output1 = false;
boolean trigger = false;

int cycle = 0;

#define MODE   3
#define PERIOD 100

void loop()
{
  long start = millis();
  int slave;

  int light_value = analogRead(PIN_LIGHT_SENSE);
  DEBUG_VALUELN(DEBUG_HIGH, F("light_value:"), light_value);
  if (light_value > 120) {
    output1 = true;
    digitalWrite(PIN_OUTPUT1, HIGH); 
  } else if (light_value > 40 && light_value < 80) {
    if (output1) {
      //analogWrite(PIN_OUTPUT1, 64);
      trigger = !trigger;
      if (trigger) digitalWrite(PIN_OUTPUT1, HIGH);
      else digitalWrite(PIN_OUTPUT1, LOW);
    }
    DEBUG_VALUELN(DEBUG_HIGH, F("Light trigger:"), trigger);
  } else if (light_value < 40) {
    output1 = false;
    digitalWrite(PIN_OUTPUT1, LOW);
  }

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
  int output;

  switch (MODE) {
      case 0:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          output_value[slave][0] = (output_value[slave][0] == 0 ? 255 : 0);
        }
        break;
      case 1:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          output_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 2:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          output_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
          output_value[slave][1] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 3:
        static int current_output = 0;
        static int current_slave = 0;
        static boolean current_on = true;

        output_value[current_slave][current_output] = (current_on ? 255 : 0);

        if (!current_on) {
          current_output = (current_output + 1) % NUM_OUTPUTS;
          if (current_output == 0)
            current_slave = (current_slave + 1) % NUM_SLAVES;
        }
        current_on = !current_on;
        break;
      case 4:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          for (output = 0; output < NUM_OUTPUTS; output++) {
            output_value[slave][output] =
              (output_value[slave][output] + 10) % 256;
          }
        }

        break;
  }
}

/* Send a state update to the indicated device */
void send_state(int slave) 
{
  /* Fill in the send buffer with this address's output state */
  msg_output_value_t *msg;
  unsigned int msg_len = 0;
  DEBUG_VALUE(DEBUG_HIGH, F("send_state:"), slave);
  for (int output = 0; output < NUM_OUTPUTS; output++) {
    if (msg_len > (MAX_MSG_DATA - sizeof (msg_output_value_t))) {
      /* Prevent buffer overflow */
      DEBUG_ERR(F("ERROR: send_state: msg exceeded length"));
      break;
    }

    msg = (msg_output_value_t *)(&send_buffer[0] +
                                 output * sizeof (msg_output_value_t));
    msg_len += sizeof (msg_output_value_t);

    msg->output = slave_output[slave][output];
    msg->value = output_value[slave][output];
    DEBUG_VALUE(DEBUG_HIGH, " ", msg->value);
  }
  DEBUG_PRINT_END();

  rs485.sendMsgTo(slave_id[slave], send_buffer, msg_len);
}
