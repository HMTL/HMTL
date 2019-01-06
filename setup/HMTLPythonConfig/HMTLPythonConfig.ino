/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * This sketch, when paired with HMTLConfig.py, is used to configure the EEPROM 
 * configuration of an HMTL based device from data received over serial.
 */
#include "Arduino.h"

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include "HMTLProtocol.h"
#include "HMTLTypes.h"
#include "GeneralUtils.h"

// XXX: Some portion of these may be required, sometimes...
#include "EEPROM.h"

#ifdef __AVR__
  #include <SoftwareSerial.h>
#endif

#include "SPI.h"
#include "FastLED.h"
#include "EEPromUtils.h"
#include "PixelUtil.h"
#include "Wire.h"

#ifdef USE_MPR121
  #include "MPR121.h"
#endif
  #include "Socket.h"
#ifdef USE_RS485
  #include "RS485Utils.h"
#endif
#ifdef USE_XBEE
  #include "XBee.h"
  #include "XBeeSocket.h"
#endif
#ifdef USE_RFM69
  #include "RFM69Socket.h"
#endif


#if defined(__AVR_ATmega32U4__)
  #define DEBUG_PIN 17
#else
  #define DEBUG_PIN LED_BUILTIN
#endif

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

/*
 * Intermittently send a ready indicator.  This is necessary for boards that
 * might not auto-reset on a serial connections (including ATMega32u4 boards).
 */
unsigned long last_ready = 0;
void print_ready() {
  unsigned long now = millis();
  if (now - last_ready > 1000) {
    Serial.println(HMTL_READY); // Indicates that module is ready for commands
    last_ready = now;
  }
}

void loop()
{
  print_ready();
  
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

  config_outputs = config_hdr.num_outputs;

  // Fill in the output array
  for (int i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    if (i < config_outputs) {
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
  int command_len = 0;

  while (Serial.available()) {
    buff[offset] = Serial.read();
    DEBUG5_HEXVAL("Recv char:", buff[offset]);
    DEBUG5_VALUELN(" off:", offset);

    /*
     * Check for termination.  This can come in two forms:
     *   1) If the first character is a potential letter, '\n' can terminate
     *   2) The 4byte HMTL_TERMINATOR
     */
    if ((offset >= 1) &&
        (buff[0] >= 'A') && (buff[0] <= 'z') &&
        (buff[offset] == '\n')) {
      command_len = offset;
      received_command = true;
    } else if ((offset >= 3) &&
               ((uint32_t)buff[offset - 3] << 24 |
                (uint32_t)buff[offset - 2] << 16 |
                (uint32_t)buff[offset - 1] << 8 |
                (uint32_t)buff[offset]) == HMTL_TERMINATOR) {
      command_len = offset - 3;
      received_command = true;
    }

    if (received_command) {
      // Replace the terminator with string-end
      buff[command_len] = '\0';
      handle_command(buff, command_len);
      offset = 0;
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

void add_output(void *config, uint16_t size) {
  memcpy(&rawoutputs[config_outputs], config, size);
  rawoutputs[config_outputs].hdr.output = config_outputs;
  outputs[config_outputs] = (output_hdr_t *)&rawoutputs[config_outputs];
  config_outputs++;
}

/*
 * Process a complete command
 */
void handle_command(byte *cmd, byte len) {
  DEBUG4_VALUE("Recv: ", (char *)cmd);
  DEBUG4_PRINT(" Hex:");
  for (int i = 0; i < len; i++) {
    DEBUG4_HEXVAL(" ", cmd[i]);
  }
  DEBUG4_PRINTLN("");

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
        DEBUG3_PRINTLN("Received configuration header");
        if (config_length != sizeof (config_hdr_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message wrong len for header: ",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_hdr_t));
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
        DEBUG3_PRINTLN("Received value output");
        if (config_length != sizeof (config_value_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for value:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_value_t));
          goto FAIL;
        }
        config_value_t *val = (config_value_t *)config_start;
        hmtl_print_output(&val->hdr);

        if (!hmtl_validate_value(val)) {
          DEBUG_ERR("Recieved invalid value output");
          goto FAIL;
        }

        add_output(val, sizeof (config_value_t));
        break;
      }
      case HMTL_OUTPUT_RGB: {
        DEBUG3_PRINTLN("Received RGB output");
        if (config_length != sizeof (config_rgb_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for RGB:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_rgb_t));
          goto FAIL;
        }
        config_rgb_t *rgb = (config_rgb_t *)config_start;
        hmtl_print_output(&rgb->hdr);

        if (!hmtl_validate_rgb(rgb)) {
          DEBUG_ERR("Recieved invalid rgb output");
          goto FAIL;
        }

        add_output(rgb, sizeof (config_rgb_t));
        break;
      }
      case HMTL_OUTPUT_PIXELS: {
        DEBUG3_PRINTLN("Received PIXELS output");
        if (config_length != sizeof (config_pixels_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for PIXELS:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_pixels_t));
          goto FAIL;
        }
        config_pixels_t *pixels = (config_pixels_t *)config_start;
        hmtl_print_output(&pixels->hdr);

        if (!hmtl_validate_pixels(pixels)) {
          DEBUG_ERR("Recieved invalid pixels output");
          goto FAIL;
        }

        add_output(pixels, sizeof (config_pixels_t));
        break;
      }
      case HMTL_OUTPUT_MPR121: {
        DEBUG3_PRINTLN("Received MPR121 output");
        if (config_length != sizeof (config_mpr121_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for MPR121:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_mpr121_t));
          goto FAIL;
        }
        config_mpr121_t *mpr121 = (config_mpr121_t *)config_start;
        hmtl_print_output(&mpr121->hdr);

        if (!hmtl_validate_mpr121(mpr121)) {
          DEBUG_ERR("Recieved invalid mpr121 output");
          break;
        }

        add_output(mpr121, sizeof (config_mpr121_t));
        break;
      }
      case HMTL_OUTPUT_RS485: {
        DEBUG3_PRINTLN("Received RS485 output");
        if (config_length != sizeof (config_rs485_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for RS485:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_rs485_t));
          goto FAIL;
        }
        config_rs485_t *rs485 = (config_rs485_t *)config_start;
        hmtl_print_output(&rs485->hdr);

        if (!hmtl_validate_rs485(rs485)) {
          DEBUG_ERR("Recieved invalid rs485 output");
          goto FAIL;
        }

        add_output(rs485, sizeof (config_rs485_t));
        break;
      }

      case HMTL_OUTPUT_XBEE: {
        DEBUG3_PRINTLN("Received XBEE output");
        if (config_length != sizeof (config_xbee_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for XBEE:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof (config_xbee_t));
          goto FAIL;
        }
        config_xbee_t *xbee = (config_xbee_t *)config_start;
        hmtl_print_output(&xbee->hdr);

        if (!hmtl_validate_xbee(xbee)) {
          DEBUG_ERR("Recieved invalid xbee output");
          goto FAIL;
        }

        add_output(xbee, sizeof (config_xbee_t));
        break;
      }

      case HMTL_COMMAND_ADDRESS: {
        if (config_length != sizeof(uint16_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for address:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof(uint16_t));
          goto FAIL;
        }
        uint16_t address = *(uint16_t *)config_start;
        DEBUG3_VALUELN("Received address: ", address);

        config_hdr.address = address;

        break;
      }

      case HMTL_COMMAND_DEVICE_ID: {
        if (config_length != sizeof(uint16_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for device_id:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof(uint16_t));
          goto FAIL;
        }
        uint16_t device_id = *(uint16_t *)config_start;
        DEBUG3_VALUELN("Received device_id: ", device_id);

        config_hdr.device_id = device_id;

        break;
      }

      case HMTL_COMMAND_BAUD: {
        if (config_length != sizeof(uint8_t)) {
          DEBUG_VALUE(DEBUG_ERROR,
                      "Received config message with wrong len for baud:",
                      config_length);
          DEBUG1_VALUELN(" needed:", sizeof(uint8_t));
          goto FAIL;
        }
        uint8_t baud = *(uint8_t *)config_start;
        DEBUG3_VALUE("Received baud byte: ", baud);
        DEBUG3_VALUELN(" baud: ", BYTE_TO_BAUD(baud));

        config_hdr.baud = baud;

        break;
      }

      default: {
        DEBUG1_VALUELN("Received unknown configuration type:",
                       type);
        goto FAIL;
      }
    }
  } else {
    /* Treat this as a plain text command */
    char *str = (char *)cmd;

    if (strcmp(str, HMTL_CONFIG_START) == 0) {
      DEBUG3_PRINTLN("Received command 'start'");
      state = STATE_READY;
    }

    else if (strcmp(str, HMTL_CONFIG_END) == 0) {
      DEBUG3_PRINTLN("Received command 'end'");
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
      DEBUG3_PRINTLN("Received command 'write'");

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
      DEBUG3_PRINTLN("Received command 'print'");
      hmtl_print_config(&config_hdr, outputs);
    }

    else if (strcmp(str, HMTL_CONFIG_READ) == 0) {
      DEBUG3_PRINTLN("Received command 'read'");
      if (!read_configuration()) {
        goto FAIL;
      }
    }

    else if (strcmp(str, HMTL_CONFIG_DUMP) == 0) {
      DEBUG3_PRINTLN("Received command 'dump'");
      hmtl_dump_config();
    }

    else {
      DEBUG1_VALUELN("Received unknown command: ", str);
      goto FAIL;
    }
  }

  // Reply with ACK
  Serial.println(HMTL_ACK);
  return;

 FAIL:
  // Reply with FAIL
  Serial.println(HMTL_FAIL);
  state = STATE_ERROR;
}
