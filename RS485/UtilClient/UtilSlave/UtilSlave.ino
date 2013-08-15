
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>


#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "RS485Utils.h"


#define PIN_RS485_1     2
#define PIN_RS485_2     7 // XXX: This changed from 3 on the old ones
#define PIN_RS485_3     4

#define PIN_DEBUG_LED  13

RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

// Pixel strand outputs
Adafruit_WS2801 pixels;

#define MAX_OUTPUTS 3
config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];
config_max_t readoutputs[MAX_OUTPUTS];



#define NUM_OUTPUTS 3
byte output_to_pin[NUM_OUTPUTS] = {
  10, 11, 12
};
byte output_value[NUM_OUTPUTS] = {
  0, 0, 0,
};

#define MAX_OUTPUTS 3
config_hdr_t config;
config_max_t outputs[MAX_OUTPUTS];

void setup()
{
  Serial.begin(9600);

  /* Attempt to read the configuration */
  if (hmtl_read_config(&config, outputs, MAX_OUTPUTS) < 0) {
    hmtl_default_config(&config);
    DEBUG_PRINTLN(DEBUG_LOW, "Using default config");
  }
  DEBUG_COMMAND(DEBUG_HIGH, hmtl_print_config(&config, outputs));

  // XXX - Continue to convert to reading outputs

  for (int output_index = 0; output_index < NUM_OUTPUTS; output_index++) {
    int pin = output_to_pin[output_index];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  pinMode(PIN_DEBUG_LED, OUTPUT);  // driver output enable

  rs485.setup();

  DEBUG_VALUELN(DEBUG_HIGH, "address=", config.address);
}


boolean b = false;

boolean update;

void loop() {
  update = false;

  /* Check for messages to this address */
  read_state();

  /* If state changed then updated all outputs */
  if (update) {
    for (int output_index = 0; output_index < NUM_OUTPUTS; output_index++) {
      int pin = output_to_pin[output_index];
      if (pin_is_PWM(pin)) {
        if (output_value[pin] == 0) digitalWrite(pin, LOW);
        else if (output_value[pin] == 255) digitalWrite(pin, HIGH);
        else analogWrite(pin, output_value[pin]);
      } else {
        if (output_value[pin] > 0) digitalWrite(pin, HIGH);
        else digitalWrite(pin, LOW);
      }
    }
  }

  blink_value(PIN_DEBUG_LED, config.address, 500, 4);

  //delay(100);
}

void read_state() 
{
  unsigned int msglen;

  const byte *data = rs485.getMsg(config.address, &msglen);

  if (data != NULL) {
    if (msglen < sizeof (msg_output_value_t)) {
      DEBUG_ERR("ERROR: msglen less than message size");
      return;
    }

    msg_output_value_t *msg = (msg_output_value_t *)data;
    unsigned int processed = 0;

    DEBUG_VALUE(DEBUG_HIGH, "read_state:", msglen);
    do {
      if (processed > (msglen - sizeof (msg_output_value_t))) {
        DEBUG_ERR("ERROR: read_state: msg_len wasn't multiple of "
                  "msg_output_value_t");
        break;
      }

      DEBUG_HEXVAL(DEBUG_HIGH, " ", msg->output);
      DEBUG_HEXVAL(DEBUG_HIGH, ":", msg->value);

      if (msg->output > NUM_OUTPUTS) {
        /* XXX: Just print something if the output is invalid? */
        DEBUG_PRINT(DEBUG_HIGH, "*");
      } else {
        output_value[output_to_pin[msg->output]] = msg->value;
      }

      processed += sizeof (msg_output_value_t);
      msg = msg + 1;
    } while (processed < msglen);
    Serial.println();

    update = true;
  }
}
