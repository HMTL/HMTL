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
    delay(50);
    digitalWrite(DEBUG_PIN, LOW);
  }
}

#define BUFF_LEN 128 + sizeof(HMTL_TERMINATOR)
boolean handle_command()
{
  static byte buff[BUFF_LEN];
  static byte offset = 0;
  boolean received_command = false;

  while (Serial.available()) {
    buff[offset] = Serial.read();
    DEBUG_HEXVAL(DEBUG_TRACE, "Recv char:", buff[offset]);
    DEBUG_VALUELN(DEBUG_TRACE, " off:", offset);

    // Check for termination
    if ((offset >= 3) &&
	((uint32_t)buff[offset - 3] << 24 |
	 (uint32_t)buff[offset - 2] << 16 |
	 (uint32_t)buff[offset - 1] << 8 |
	 (uint32_t)buff[offset]) == HMTL_TERMINATOR) {
      // Replace the terminator with string-end
      buff[offset - 3] = '\0';

      handle_command(buff, offset - 3); // ??? Whats the correct length?
      offset = 0;
      received_command = true;
    } else {
      offset++;
      if (offset >= BUFF_LEN) offset = 0;
    }
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
	DEBUG_ERR("Recieved invalid value output");
	break;
      }

      memcpy(&rawoutputs[config_outputs], val, sizeof (config_value_t));
      rawoutputs[config_outputs].hdr.output = config_outputs;
      outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
      config_outputs++;
      break;
    }
    case HMTL_OUTPUT_RGB: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received RGB output");
      if (config_length != sizeof (config_rgb_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message with wrong len for RGB:",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_rgb_t));
	break;
      }
      config_rgb_t *rgb = (config_rgb_t *)config_start;
      hmtl_print_output(&rgb->hdr);

      if (!hmtl_validate_rgb(rgb)) {
	DEBUG_ERR("Recieved invalid rgb output");
	break;
      }

      memcpy(&rawoutputs[config_outputs], rgb, sizeof (config_rgb_t));
      rawoutputs[config_outputs].hdr.output = config_outputs;
      outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
      config_outputs++;
      break;
    }
    case HMTL_OUTPUT_PIXELS: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received PIXELS output");
      if (config_length != sizeof (config_pixels_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message with wrong len for PIXELS:",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_pixels_t));
	break;
      }
      config_pixels_t *pixels = (config_pixels_t *)config_start;
      hmtl_print_output(&pixels->hdr);

      if (!hmtl_validate_pixels(pixels)) {
	DEBUG_ERR("Recieved invalid pixels output");
	break;
      }

      memcpy(&rawoutputs[config_outputs], pixels, sizeof (config_pixels_t));
      rawoutputs[config_outputs].hdr.output = config_outputs;
      outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
      config_outputs++;
      break;
    }
    case HMTL_OUTPUT_RS485: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received RS485 output");
      if (config_length != sizeof (config_rs485_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message with wrong len for RS485:",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_rs485_t));
	break;
      }
      config_rs485_t *rs485 = (config_rs485_t *)config_start;
      hmtl_print_output(&rs485->hdr);

      if (!hmtl_validate_rs485(rs485)) {
	DEBUG_ERR("Recieved invalid rs485 output");
	break;
      }

      memcpy(&rawoutputs[config_outputs], rs485, sizeof (config_rs485_t));
      rawoutputs[config_outputs].hdr.output = config_outputs;
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
