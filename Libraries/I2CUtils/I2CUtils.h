#ifndef I2CUTILS_H
#define I2CUTILS_H

#include <Arduino.h>
#include "EEPROM.h"

/*
 * Setting or reading I2C address from EEProm
 */
#define I2C_ADDRESS_MAGIC 0x53
#define I2C_ADDRESS_START 0x101

void blink_value(int pin, int value, int period_ms, int idle_periods) ;

int I2C_read_address();
void I2Cwrite_address(int address);
int I2C_read_or_write_address(int address);

/*
 * Sending and receiving I2C data
 */
void I2C_write_struct(byte *data, int data_len);
void I2C_read_struct(byte *data, int data_len);

/*
 * Module message format
 */
typedef struct {
  byte pin;
  byte value;
} message_t;


/* Check if a pin is PWM */
boolean pin_is_PWM(int pin);

#endif
