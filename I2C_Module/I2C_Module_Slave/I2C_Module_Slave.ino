
#include <Wire.h>
#include <EEPROM.h>

#include <I2CModule.h>
#include <I2CUtils.h>

#define PIN_DEBUG_LED  13

I2CM_Slave slave("test");

void setup() {
  Serial.begin(9600);

  // Get the address from EEProm
  int read_address = I2C_read_or_write_address(my_address);
  if (read_address != -1) {
    my_address = read_address;
  }
  
  Wire.begin(my_address);
  Wire.onReceive(receiveEvent); // register event  
}

void loop() 
{
  delay(1000);
}


void receiveMessage(int msg_len) 
{
  I2CM_message_t hdr;

  if (msg_len < sizeof (I2CM_message_t)) {
    DEBUG_PRINTLN(DEBUG_LOW, "recv: len less than header");
    return;
  }

  I2C_read_struct((byte *)&msg, sizeof (hdr));
  if (msg.start != I2CM_START) {
    DEBUG_PRINTLN(DEBUG_LOW, "recv: invalid start code");
    return;
  }
}
