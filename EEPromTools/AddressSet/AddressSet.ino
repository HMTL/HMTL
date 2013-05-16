/*
 * Write out an I2C address if one doesn't already exist
 */
#include <EEPROM.h>

#define PIN_DEBUG_LED 13

#define HMTL_ADDRESS_MAGIC 0x53
#define HMTL_ADDRESS_START 0x101

byte my_address = 2; // This value is written to EEPROM
boolean wrote_address = false;

void setup() 
{
  Serial.begin(9600);

  // Check if there is already an address
  int value = EEPROM.read(HMTL_ADDRESS_START);
  if (value != HMTL_ADDRESS_MAGIC) {
    // No existing address, write it out
    EEPROM.write(HMTL_ADDRESS_START, HMTL_ADDRESS_MAGIC);
    EEPROM.write(HMTL_ADDRESS_START + 1, my_address);
    wrote_address = true;
  } else {
    // Read in the pre-existing address
    my_address = EEPROM.read(HMTL_ADDRESS_START + 1);
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

  blinkValue(my_address, 500, 4);
  delay(10);
}

void blinkValue(int value, int period_ms, int idle_periods) 
{
  static long period_start_ms = 0;
  static long last_step_ms = 0;
  static boolean led_value = true;

  long now = millis();
  if (period_start_ms == 0) period_start_ms = now;

  if ((now - last_step_ms) > period_ms) {
    int current_step = (now - period_start_ms) / period_ms;

    if (current_step >= (value * 2 + idle_periods)) {
      // The end of the current display cycle
      period_start_ms = now;
    } else if (current_step >= value * 2) {
      return;
    }

    led_value = !led_value;
    digitalWrite(PIN_DEBUG_LED, led_value);

    last_step_ms = now;
  }
  
}
