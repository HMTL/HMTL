#ifndef I2CUTILS_H
#define I2CUTILS_H

#include <Arduino.h>

#define I2C_ADDRESS_MAGIC 0x53
#define I2C_ADDRESS_START 0x101

void blink_value(int pin, int value, int period_ms, int idle_periods) ;

int I2C_read_address();
void I2Cwrite_address(int address);
int I2C_read_or_write_address(int address);


#endif
