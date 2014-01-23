#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Wire.h"
#include "Adafruit_WS2801.h"


#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

/* Pin definitions */
#define PIN_LIGHT_SENSE A0

#define PIN_RS485_1     2
#define PIN_RS485_2     7 // XXX: This changed from 3 on the old ones
#define PIN_RS485_3     4

#define PIN_DEBUG_LED  13

RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

// Pixel strand outputs
PixelUtil pixels;

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_max_t readoutputs[HMTL_MAX_OUTPUTS];

#define SEND_BUFFER_SIZE (sizeof (rs485_socket_msg_t) + sizeof (msg_hdr_t) + sizeof (msg_max_t) + 64)

byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

/* Set if a command was received to update an output */
boolean output_updated[HMTL_MAX_OUTPUTS];

void setup() 
{
  Serial.begin(9600);
  
  /* Attempt to read the configuration */
  if (hmtl_read_config(&config, readoutputs, HMTL_MAX_OUTPUTS) < 0) {
    hmtl_default_config(&config);
    DEBUG_PRINTLN(DEBUG_LOW, "Using default config");
  }
  if (config.num_outputs > HMTL_MAX_OUTPUTS) {
    DEBUG_VALUELN(0, "Too many outputs:", config.num_outputs);
    config.num_outputs = HMTL_MAX_OUTPUTS;
  }
  for (int i = 0; i < config.num_outputs; i++) {
    outputs[i] = (output_hdr_t *)&readoutputs[i];
  }
  DEBUG_COMMAND(DEBUG_HIGH, hmtl_print_config(&config, outputs));

  /* Initialize the outputs */
  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_setup_output((output_hdr_t *)outputs[i], &pixels);
    output_updated[i] = false;
  }

  /* Initialize debug LED */
  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);

  if (config.flags & HMTL_FLAG_MASTER) {
    // XXX - Any master specific setup?
  }
  if (config.flags & HMTL_FLAG_SERIAL) {
    // XXX - Any serial command specific setup
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);
}


void loop() 
{
  if (config.flags & HMTL_FLAG_SERIAL) {
    // TODO
  }

  if (config.flags & HMTL_FLAG_MASTER) {
    // TODO: Send commands to slaves
  } else {
    // TODO: Read commands
  }

#if 1
  /* Populate the outputs with test data */
  for (int i = 0; i < config.num_outputs; i++) {
    hmtl_test_output_car(outputs[i], &pixels);
    output_updated[i] = true;
  }
#endif

  /* Update the state of the outputs */
  for (int i = 0; i < config.num_outputs; i++) {
    if (output_updated[i]) {
      output_hdr_t *out = outputs[i];
      hmtl_update_output(out, &pixels);
      output_updated[i] = false;
    }
  }

  blink_value(PIN_DEBUG_LED, config.address, 500, 4);
  delay(10); // XXX
}

//void send_output(byte address, 
