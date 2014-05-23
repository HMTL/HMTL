/*
 * Basic RS485 socket send test
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

  int32_t outputs_found = hmtl_setup(&config, readoutputs, 
				     outputs, NULL, MAX_OUTPUTS,
				     &rs485, NULL, &rgb_output, &value_output,
				     NULL);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL Send test initialized");
}

int cycle = 0;

void loop() {

  // Send the current cycle to the remote
  send_buffer[0] = 'A';
  send_buffer[1] += 1;

  DEBUG_VALUE(DEBUG_HIGH, "Sending: ", (char)send_buffer[0]);
  DEBUG_VALUELN(DEBUG_HIGH, " - ", send_buffer[1]);

  rs485.sendMsgTo(RS485_ADDR_ANY, send_buffer, 2);

  delay(1000);
  cycle++;
}
