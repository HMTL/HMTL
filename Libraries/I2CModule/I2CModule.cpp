//#define DEBUG_LEVEL DEBUG_HIGH

#include <Arduino.h>

#include "I2CUtil.h"
#include "Debug.h"

/* Initialize the module, reading its address in from EEPROM */
I2CM_Slave::I2CM_Slave(char *_name) 
{
  memcpy(info.name, _name, sizeof (name));
  info.address = I2C_read_address();

  DEBUG_PRINT(DEBUG_LOW, "I2CM init");
  DEBUG_PRINT(DEBUG_LOW, info.name);
  DEBUG_PRINT(DEBUG_LOW, "-");
  DEBUG_PRINTLN(DEBUG_LOW, info.address);
}


I2CM_Slave::I2CM_Slave(char *_name, int _address) 
{
  memcpy(info.name, _name, sizeof (name));
  info.address = _address;
}

void I2CM_Slave::send_intro() 
{
  I2CM_message_t msg = {
    I2CM_START,
    I2CM_TYPE_INTRO,
    sizeof (info)
  };
  
  Wire.beginTransmission(); // XXX - Lookup correct way to respond
  I2C_write_struct((byte *)&msg, sizeof (msg));
  I2C_write_struct((byte *)&info, sizeof (info));
  Wire.endTransmission();
}
