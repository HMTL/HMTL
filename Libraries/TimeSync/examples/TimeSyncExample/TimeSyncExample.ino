/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * This sketch provides a debugging interface for HMTL 
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
#include "XBee.h"
#include "XBeeSocket.h"
// End note

#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLProtocol.h"
#include "TimeSync.h"

#define NUM_OUTPUTS 3
config_rgb_t rgb_output;


#define SEND_BUFFER_SIZE 64
byte databuffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];
byte *send_buffer;
RS485Socket rs485;

SerialCLI serialcli(64, cliHandler);

config_hdr_t config;

TimeSync time = TimeSync();

void setup() {
  Serial.begin(57600);
  DEBUG2_PRINTLN("*** TimeSyncExample starting ***");

  config_max_t readoutputs[HMTL_MAX_OUTPUTS];
  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     NULL, NULL, HMTL_MAX_OUTPUTS,
                                     &rs485,
                                     NULL, // XBee
                                     NULL, // Pixels
                                     NULL, // MPR121
                                     &rgb_output,
                                     NULL, // Value
                                     NULL);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer, SEND_BUFFER_SIZE);

  DEBUG2_VALUELN("*** TimeSyncExample initialized.  Address:", config.address);
  print_usage();
}

unsigned long last_print = 0;
void loop() {

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr_t *msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, config.address);
  if ((msg_hdr != NULL) && (msg_hdr->type == MSG_TYPE_TIMESYNC)) {
    time.synchronize(&rs485, SOCKET_ADDR_INVALID, msg_hdr);
  }

  /* Handle commands from the Serial CLI connection */
  serialcli.checkSerial();

  if (millis() > last_print + 1000) {
    DEBUG1_VALUELN("Time:", time.ms());
    last_print = millis();
  }
}

void print_usage() {
Serial.print(F(
  " \n"
  "Usage:\n"
  "  h - print this help\n"
  "  t <addr> - Initiate time sync\n"
  "  r <addr> - Initiate time resync\n"
  "  c <addr> - Send a time check\n"
  "  s <time> - Set the time\n"
               ));
}             
void cliHandler(char **tokens, byte numtokens) {
  switch (tokens[0][0]) {

    case 'h': {
      print_usage();
      break;
    }

    case 't': {
      if (numtokens < 1) return;
      uint16_t address = (numtokens >= 2 ? atoi(tokens[1]) : SOCKET_ADDR_ANY); 
      DEBUG1_VALUE("* Time sync to: ", address);
      time.synchronize(&rs485, address, NULL);
      break;
    }

    case 'r': {
      if (numtokens < 1) return;
      uint16_t address = (numtokens >= 2 ? atoi(tokens[1]) : SOCKET_ADDR_ANY); 
      DEBUG1_VALUE("* Time resync to: ", address);
      time.resynchronize(&rs485, address);
      break;
    }

    case 's': {
      if (numtokens < 2) return;
      uint32_t t = atol(tokens[1]);
      DEBUG1_VALUE("* Time set to: ", t);
      time.set(t);
      break;
    }

    case 'c': {
      if (numtokens < 1) return;
      uint16_t address = (numtokens >= 2 ? atoi(tokens[1]) : SOCKET_ADDR_ANY); 
      DEBUG1_VALUE("* Time check to: ", address);
      time.check(&rs485, address);
      break;
    }


  }

  DEBUG_PRINT_END();
}
