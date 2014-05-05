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
#include "Wire.h"


#define DEBUG_PIN 13

config_hdr_t config_hdr;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_max_t rawoutputs[HMTL_MAX_OUTPUTS];
int config_outputs = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(DEBUG_PIN, OUTPUT);

  /* Setup the output pointer array */
  for (int i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    outputs[i] = NULL;
  }

  Serial.println(HMTL_READY); // Indicates that module is ready for commands
}

void loop()
{
  if (receive_command()) {
    digitalWrite(DEBUG_PIN, HIGH);
  } else {
    delay(50);
    digitalWrite(DEBUG_PIN, LOW);
  }
}

/* Read the current configuration from EEPROM */
boolean read_configuration() {
  int configOffset = hmtl_read_config(&config_hdr,
				      rawoutputs,
				      HMTL_MAX_OUTPUTS);
  if (configOffset < 0) {
    DEBUG_ERR("Failed to read configuration");
    return false;
  }

  // Fill in the output array
  for (int i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    if (i < config_hdr.num_outputs) {
      outputs[i] = &rawoutputs[i].hdr;
    } else {
      outputs[i] = NULL;
    }
  }

  return true;
}

/*
 * Read in commands from the serial device
 */
#define BUFF_LEN 128 + sizeof(HMTL_TERMINATOR)
boolean receive_command()
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

/* State machine for receiving and writing a configuration */
#define STATE_NEW    0
#define STATE_READY  1
#define STATE_DONE   2
#define STATE_ERROR -1
int state = STATE_NEW;

/*
 * Process a complete command
 */
void handle_command(byte *cmd, byte len) {
  DEBUG_VALUE(DEBUG_HIGH, "Recv: ", (char *)cmd);
  DEBUG_PRINT(DEBUG_HIGH, " Hex:");
  for (int i = 0; i < len; i++) {
    DEBUG_HEXVAL(DEBUG_HIGH, " ", cmd[i]);
  }
  DEBUG_PRINTLN(DEBUG_HIGH, "");

  if (cmd[0] == CONFIG_START_BYTE) {
    /* This is a configuration command */

    if (state != STATE_READY) {
      DEBUG_ERR("Received configuration before START");
      goto FAIL;
    }

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
	goto FAIL;
      }
      config_hdr_t *hdr = (config_hdr_t *)config_start;
      hmtl_print_header(hdr);

      if (!hmtl_validate_header(hdr)) {
	DEBUG_ERR("Recieved invalid hdr");
	goto FAIL;
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
	goto FAIL;
      }
      config_value_t *val = (config_value_t *)config_start;
      hmtl_print_output(&val->hdr);

      if (!hmtl_validate_value(val)) {
	DEBUG_ERR("Recieved invalid value output");
	goto FAIL;
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
	goto FAIL;
      }
      config_rgb_t *rgb = (config_rgb_t *)config_start;
      hmtl_print_output(&rgb->hdr);

      if (!hmtl_validate_rgb(rgb)) {
	DEBUG_ERR("Recieved invalid rgb output");
	goto FAIL;
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
	goto FAIL;
      }
      config_pixels_t *pixels = (config_pixels_t *)config_start;
      hmtl_print_output(&pixels->hdr);

      if (!hmtl_validate_pixels(pixels)) {
	DEBUG_ERR("Recieved invalid pixels output");
	goto FAIL;
      }

      memcpy(&rawoutputs[config_outputs], pixels, sizeof (config_pixels_t));
      rawoutputs[config_outputs].hdr.output = config_outputs;
      outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
      config_outputs++;
      break;
    }
    case HMTL_OUTPUT_MPR121: {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received MPR121 output");
      if (config_length != sizeof (config_mpr121_t)) {
	DEBUG_VALUE(DEBUG_ERROR,
		    "Received config message with wrong len for MPR121:",
		    config_length);
	DEBUG_VALUELN(DEBUG_ERROR, " needed:", sizeof (config_mpr121_t));
	goto FAIL;
      }
      config_mpr121_t *mpr121 = (config_mpr121_t *)config_start;
      hmtl_print_output(&mpr121->hdr);

      if (!hmtl_validate_mpr121(mpr121)) {
	DEBUG_ERR("Recieved invalid mpr121 output");
	break;
      }

      memcpy(&rawoutputs[config_outputs], mpr121, sizeof (config_mpr121_t));
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
	goto FAIL;
      }
      config_rs485_t *rs485 = (config_rs485_t *)config_start;
      hmtl_print_output(&rs485->hdr);

      if (!hmtl_validate_rs485(rs485)) {
	DEBUG_ERR("Recieved invalid rs485 output");
	goto FAIL;
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
      goto FAIL;
    }
    }
  } else {
    /* Treat this as a plain text command */
    char *str = (char *)cmd;

    if (strcmp(str, HMTL_CONFIG_START) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config start");
      state = STATE_READY;
    }

    else if (strcmp(str, HMTL_CONFIG_END) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config end");
      if (state != STATE_READY) {
	DEBUG_ERR("Recived END before START");
	goto FAIL;
      }

      /* Validate the configuration */
      if (!hmtl_validate_config(&config_hdr, outputs, config_outputs)) {
	DEBUG_ERR("Overall configuration was not valid");
	goto FAIL;
      }

      state = STATE_DONE;
    }

    else if (strcmp(str, HMTL_CONFIG_WRITE) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config write");

      if (state != STATE_DONE) {
	DEBUG_ERR("Received WRITE when state is not DONE");
	goto FAIL;
      }

      int configOffset = hmtl_write_config(&config_hdr, outputs);
      if (configOffset < 0) {
	DEBUG_ERR("Failed to write configuration");
	goto FAIL;
      }
    }

    else if (strcmp(str, HMTL_CONFIG_PRINT) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config print");
      hmtl_print_config(&config_hdr, outputs);
    }

    else if (strcmp(str, HMTL_CONFIG_READ) == 0) {
      DEBUG_PRINTLN(DEBUG_HIGH, "Received config read");
      if (!read_configuration()) {
	goto FAIL;
      }
    }
  }

  // Reply with ACK
  Serial.println(HMTL_ACK);
  return;

 FAIL:
  // Reply with FAIL
  Serial.print(HMTL_FAIL);
  state = STATE_ERROR;
}
