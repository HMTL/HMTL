#include <Arduino.h>
#include "EEPROM.h"

#include "I2CUtils.h"

void blink_value(int pin, int value, int period_ms, int idle_periods) 
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
    digitalWrite(pin, led_value);

    last_step_ms = now;
  }
}


int I2C_read_address() 
{
  int value = EEPROM.read(I2C_ADDRESS_START);
  if (value != I2C_ADDRESS_MAGIC) {
    return -1;
  } else {
    return EEPROM.read(I2C_ADDRESS_START + 1);
  }
}

void I2C_write_address(int address) 
{
  EEPROM.write(I2C_ADDRESS_START, I2C_ADDRESS_MAGIC);
  EEPROM.write(I2C_ADDRESS_START + 1, address);
}

/*
 * Write the address only if one isn't present, returns the address read or
 * -1 if one was written
 */
int I2C_read_or_write_address(int set_address) 
{
  int address = I2C_read_address();
  if (address != -1) return address;
  I2C_write_address(set_address);
  return -1;
}

