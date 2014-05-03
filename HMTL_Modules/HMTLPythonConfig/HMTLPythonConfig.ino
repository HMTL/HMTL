#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "HMTLprotocol.h"
#include "HMTLTypes.h"

// XXX: Some portion of these may be required, sometimes...
#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "EEPromUtils.h"
#include "PixelUtil.h"
#include "MPR121.h"
#include "RS485Utils.h"



#define DEBUG_PIN 13

#define MAX_OUTPUTS 8

config_hdr_t config_hdr;
output_hdr_t *outputs[MAX_OUTPUTS];
config_max_t rawoutputs[MAX_OUTPUTS];
int config_outputs = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(DEBUG_PIN, OUTPUT);

  /* Setup the output pointer array */
  for (int i = 0; i < MAX_OUTPUTS; i++) {
    outputs[i] = NULL;
  }

  Serial.println(HMTL_READY); // Indicates that module is ready for commands
}

void loop()
{
  if (handle_command()) {
    digitalWrite(DEBUG_PIN, HIGH);
  } else {
    delay(500);
    digitalWrite(DEBUG_PIN, LOW);
  }
}

#define BUFF_LEN 128
boolean handle_command()
{
  static byte buff[BUFF_LEN + 1];
  static byte offset = 0;
  boolean received_command = false;

  while (Serial.available()) {
    buff[offset] = Serial.read();
    
    offset++;
    if (buff[offset - 1] == '\n') {
      // Replace the newline with termination
      buff[offset - 1] = '\0';

      handle_command(buff, offset - 1); // ??? Whats the correct length?
      offset = 0;
      received_command = true;
    }

    if (offset >= BUFF_LEN) offset = 0;
  }

  return received_command;
}

void handle_command(byte *cmd, byte len) {

  DEBUG_VALUE(DEBUG_HIGH, "Recv: ", (char *)cmd);
  DEBUG_PRINT(DEBUG_HIGH, " Hex:");
  for (int i = 0; i < len; i++) {
    DEBUG_HEXVAL(DEBUG_HIGH, " ", cmd[i]);
  }
  DEBUG_PRINTLN(DEBUG_HIGH, "");

  if (cmd[0] == CONFIG_START_BYTE) {
    /* This is a configuration command */
    byte type = cmd[1];

    // TODO: Check for valid command length

    void *config_start = &cmd[CONFIG_START_SIZE];
    int config_length = len - CONFIG_START_SIZE;
    switch (type) {
    case 0: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received configuration header");
      if (config_length != sizeof (config_hdr_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message wrong len for header: ",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_hdr_t));
	break;
      }
      config_hdr_t *hdr = (config_hdr_t *)config_start;
      hmtl_print_header(hdr);

      if (!hmtl_validate_header(hdr)) {
	DEBUG_ERR("Recieved invalid hdr");
	break;
      }

      memcpy(&config_hdr, hdr, sizeof (config_hdr_t));

      break;
    }
    case HMTL_OUTPUT_VALUE: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received value output");
      if (config_length != sizeof (config_value_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message with wrong len for value:",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_value_t));
	break;
      }
      config_value_t *val = (config_value_t *)config_start;
      hmtl_print_output(&val->hdr);

      if (!hmtl_validate_value(val)) {
	DEBUG_ERR("Recieved invalid output");
	break;
      }

      memcpy(&rawoutputs, val, sizeof (config_value_t));
      outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
      config_outputs++;
      break;
    }

    default: {
      DEBUG_VALUELN(DEBUG_ERROR, "Received unknown configuration type:",
		    type);
      break;
    }
    }
  } else {
    /* Treat this as a plain text command */
    char *str = (char *)cmd;
    if (strcmp(str, HMTL_CONFIG_START) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config start");
    } else if (strcmp(str, HMTL_CONFIG_END) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config end");
    } else if (strcmp(str, HMTL_CONFIG_PRINT) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config print");
      hmtl_print_config(&config_hdr, outputs);
    }
  }

  // Send the ACK
  Serial.println(HMTL_ACK);
}
