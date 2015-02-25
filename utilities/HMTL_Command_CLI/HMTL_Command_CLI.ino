/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * Code for a fully contained module which handles HMTL formatted messages
 * from a serial or RS485 connection.
 ******************************************************************************/

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>

#define DEBUG_LEVEL 4
#include <Debug.h>

#include "SerialCLI.h"

#include <RS485_non_blocking.h>
#include "Socket.h"
#include "RS485Utils.h"

// Note: These are only needed for ArduinoIDE compilation.  With this environment
// make sure to set DISABLE_MPR121 in the HMTLTypes.h, otherwise the static
// memory allocation (blame Wire.h) will exceed some limit and this won't work
#include "SPI.h"
#include "EEPROM.h"
#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "Wire.h"
#include "MPR121.h"
#include "FastLED.h"
#include "PixelUtil.h"
// End note

#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLProtocol.h"

#define NUM_OUTPUTS 3
config_rgb_t rgb_output;


#define SEND_BUFFER_SIZE 64
byte databuffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];
byte *send_buffer;
RS485Socket rs485;

SerialCLI serialcli(64, cliHandler);

config_hdr_t config;

void setup() {
  Serial.begin(9600);
  DEBUG2_PRINTLN("*** HMTL_Command_CLI starting ***");

  config_max_t readoutputs[HMTL_MAX_OUTPUTS];
  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     NULL, NULL, HMTL_MAX_OUTPUTS,
                                     &rs485, NULL, NULL,
                                     &rgb_output, NULL,
                                     NULL);

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG2_VALUELN("*** HMTL_Command_CLI initialized.  Address:", config.address);
  print_usage();
}

void loop() {

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr_t *msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, config.address);
  if (msg_hdr != NULL) {
    process_message(msg_hdr, msglen);
  }

  /* Handle commands from the Serial CLI connection */
  serialcli.checkSerial();
}

void print_usage() {
Serial.print(F(" \n"
  "Usage:\n"
  "  h - print this help\n"
  "  s <addr> - Send sensor check\n"
  "  p <addr> - Send poll request\n"
               ));
}             
void cliHandler(char **tokens, byte numtokens) {
  switch (tokens[0][0]) {

    case 'h': {
      print_usage();
      break;
    }

    case 's': {
      if (numtokens < 2) return;
      uint16_t address = atoi(tokens[1]);
      DEBUG1_VALUELN("* Sensor request to: ", address);
      hmtl_send_sensor_request(&rs485, send_buffer, SEND_BUFFER_SIZE, address);
      break;
    }

    case 'p': {
      if (numtokens < 2) return;
      uint16_t address = atoi(tokens[1]);
      DEBUG1_VALUELN("* Poll request to: ", address);
      hmtl_send_poll_request(&rs485, send_buffer, SEND_BUFFER_SIZE, address);
      break;
    } 

  }
}

void process_message(msg_hdr_t *msg, unsigned int msglen) {
  DEBUG1_VALUE("Recieved rs485 msg len:", msglen);
  DEBUG1_VALUE(" src:", RS485_SOURCE_FROM_DATA(msg));
  DEBUG1_VALUE(" dst:", RS485_ADDRESS_FROM_DATA(msg));
  DEBUG1_VALUE(" len:", msg->length);
  DEBUG1_VALUE(" type:", msg->type);
  DEBUG1_HEXVAL(" flags:0x", msg->flags);
  DEBUG1_PRINT(" data:");
  DEBUG1_COMMAND(
                 print_hex_string((byte *)msg, msglen)
                 );
  DEBUG_PRINT_END();

  switch (msg->type) {
    case MSG_TYPE_SENSOR: {
      DEBUG1_PRINTLN(" * Sensor data:");

      msg_sensor_data_t *sense = NULL;
      while (sense = hmtl_next_sensor(msg, sense)) {
        DEBUG1_VALUE(" ptr:", (int)sense);
        DEBUG1_VALUE(" type:", sense->sensor_type);
        DEBUG1_VALUE(" datalen:", sense->data_len);

        switch (sense->sensor_type) {
          case HMTL_SENSOR_SOUND: {
            DEBUG1_PRINT(" SOUND:");
            uint16_t *data = (uint16_t *)&sense->data;
            for (byte i = 0; i < sense->data_len / sizeof (uint16_t); i++) {
              DEBUG1_HEXVAL(" ", data[i]);
            }
            break;
          }
          case HMTL_SENSOR_LIGHT: {
            uint16_t *data = (uint16_t *)&sense->data;
            DEBUG1_VALUE(" LIGHT:", *data);
            break;
          }
          case HMTL_SENSOR_POT: {
            uint16_t *data = (uint16_t *)&sense->data;
            DEBUG1_VALUE(" POT:", *data);
            break;
          }
          default: {
            DEBUG1_PRINT(" ERROR: UNKNOWN TYPE");
            return;
          }
        }

        DEBUG_PRINT_END();
      }
      break;
    }
    case MSG_TYPE_POLL: {
      DEBUG1_PRINT("POLL");
      break;
    }
  }

  DEBUG_PRINT_END();
}
