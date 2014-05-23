/*
 * Basic RS485 socket recv test
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
config_rgb_t rgb_output; boolean has_rgb = false;
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

  has_rgb = (outputs_found & (1 << HMTL_OUTPUT_RGB));
  if (!has_rgb) {
    DEBUG_ERR("No RGB output found.");
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL Recv test initialized");
}

int cycle = 0;

void loop() {
  unsigned int msglen;
  const byte *data = rs485.getMsg(RS485_ADDR_ANY, &msglen);

  if (data != NULL) {
    DEBUG_PRINT(DEBUG_HIGH, " Recv:");
    DEBUG_COMMAND(DEBUG_HIGH, print_hex_string(data, msglen));
    DEBUG_PRINTLN(DEBUG_HIGH, "");
    
    if (has_rgb) {
      digitalWrite(rgb_output.pins[0], HIGH);
      digitalWrite(rgb_output.pins[1], HIGH);
      digitalWrite(rgb_output.pins[2], HIGH);
    }
  } else {
    digitalWrite(rgb_output.pins[0], LOW);
    digitalWrite(rgb_output.pins[1], LOW);
    digitalWrite(rgb_output.pins[2], LOW);
  }

  delay(500);
  cycle++;
}
