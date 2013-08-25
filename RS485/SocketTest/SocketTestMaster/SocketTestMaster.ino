#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "RS485Utils.h"

#define PIN_RS485_1     2
#define PIN_RS485_2     7 // XXX: This changed from 3 on the old ones
#define PIN_RS485_3     4

RS485Socket rs485(PIN_RS485_1, PIN_RS485_2, PIN_RS485_3, (DEBUG_LEVEL != 0));

#define SEND_BUFFER_SIZE (sizeof (rs485_socket_hdr_t) + 64)
byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup()
{
  Serial.begin(9600);

  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);
}

#define MY_ADDR   0
#define DEST_ADDR 1

byte count = 0;
void loop()
{
  send_buffer[0] = 'T';
  send_buffer[1] = count++;

  DEBUG_VALUELN(0, "Sending ", count);

  rs485.sendMsgTo(DEST_ADDR, send_buffer, 2);

  delay(1000);
}
