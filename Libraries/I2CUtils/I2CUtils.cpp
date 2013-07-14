#include <Arduino.h>
#include "EEPROM.h"
#include "Wire.h"

#include "I2CUtils.h"

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

/*
 * Transmit a data buffer over I2C
 */
void I2C_write_struct(byte *data, int data_len) 
{
  for (int i = 0; i < data_len; i++) {
    Wire.write(data[i]);
  }
}

void I2C_read_struct(byte *data, int len) 
{
  for (int i = 0; i < len; i++) {
    byte val = Wire.read();
    data[i] = val;
  }
}
