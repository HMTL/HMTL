/*
 * A master node
 */


#include <Wire.h>
#include <EEPROM.h>

#include <I2CUtils.h>
#include <I2CModule.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

void setup()
{
  Serial.begin(9600);

  Wire.begin(); // Start I2C Bus as Master

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, HIGH);
}

void loop() 
{

  scan_addresses();

}



/*
 * Ping all addresses to check for listening devices
 */
void scan_addresses() 
{
  I2CM_message_t msg = {
    I2CM_START,
    I2CM_TYPE_INTRO_REQUEST,
    0
  };

  for (byte address = 0; address < 256; address++) {
    Wire.beginTransmission(address);
    I2C_write_struct((byte *)&msg, sizeof (msg));
    Wire.endTransmission();
  }
}
