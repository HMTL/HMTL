/*
 * Listen for HMTL formatted messages
 */

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "Wire.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

RS485Socket rs485;
config_rgb_t rgb_output;
config_value_t value_output;

#define MAX_OUTPUTS 8
config_hdr_t config;
output_hdr_t *outputs[MAX_OUTPUTS];
config_max_t readoutputs[MAX_OUTPUTS];

#define SEND_BUFFER_SIZE (sizeof (rs485_socket_hdr_t) + 64)
byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup() {
  Serial.begin(9600);

  int32_t outputs_found = hmtl_setup(&config, readoutputs, outputs, MAX_OUTPUTS,
			     &rs485, NULL, &rgb_output, &value_output,
			     NULL);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL MSG Test initialized");
  DEBUG_PRINTLN(DEBUG_LOW, "ready")
}

int cycle = 0;

#define MSG_MAX_SZ (sizeof(msg_hdr_t) + sizeof(msg_max_t))
byte msg[MSG_MAX_SZ];
byte offset = 0;

void loop() {
  
  /* Check for messages on the serial interface */
  msg_hdr_t *msg_hdr = (msg_hdr_t *)msg;
  if (hmtl_serial_getmsg(msg, MSG_MAX_SZ, &offset)) {
    /* Received a complete message */
    DEBUG_VALUELN(DEBUG_HIGH, "Received msg len=", offset);
    DEBUG_COMMAND(DEBUG_HIGH, 
		  print_hex_string(msg, offset)
		  );
    DEBUG_PRINTLN(DEBUG_HIGH, "");
    DEBUG_PRINTLN(DEBUG_HIGH, "ok");
    offset = 0;

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      hmtl_handle_msg((msg_hdr_t *)&msg, &config, outputs);
      for (int i = 0; i < config.num_outputs; i++) {
	hmtl_update_output(outputs[i], NULL);
      }
    }

    if ((msg_hdr->address != config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      rs485.sendMsgTo(msg_hdr->address, msg, offset);
    }
  }

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, RS485_ADDR_ANY);
  if (msg_hdr != NULL) {
    DEBUG_VALUELN(DEBUG_HIGH, "Received msg len=", msglen);
    DEBUG_COMMAND(DEBUG_HIGH, 
		  print_hex_string((byte *)&msg, msglen)
		  );
    DEBUG_PRINTLN(DEBUG_HIGH, "");
    DEBUG_PRINTLN(DEBUG_HIGH, "ok");

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      hmtl_handle_msg(msg_hdr, &config, outputs);
      for (int i = 0; i < config.num_outputs; i++) {
	hmtl_update_output(outputs[i], NULL);
      }
    }
  }
}
