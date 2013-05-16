/*
 * Write out an I2C address if one doesn't already exist
 */
#include <EEPROM.h>

#include <I2CUtils.h>

#define PIN_DEBUG_LED 12

#define HMTL_ADDRESS_MAGIC 0x53
#define HMTL_ADDRESS_START 0x101

byte my_address = 2; // This value is written to EEPROM
boolean wrote_address = false;

void setup() 
{
  Serial.begin(9600);

  // Check if there is already an address
  int value = EEPROM.read(HMTL_ADDRESS_START);

  int read_address = I2C_read_or_write_address(my_address);
  if (read_address != -1) {
    my_address = read_address;
  } else {
    wrote_address = true;
  }
  
  pinMode(PIN_DEBUG_LED, OUTPUT);
}


void loop() 
{
  static long last_update = 0;
  if (millis() - last_update > 1000) {
    Serial.print("My address: ");
    Serial.print(my_address);
    Serial.print(" Was address written: ");
    Serial.println(wrote_address);
    last_update = millis();
  }

  blink_value(PIN_DEBUG_LED, my_address, 500, 4);
  delay(10);
}

