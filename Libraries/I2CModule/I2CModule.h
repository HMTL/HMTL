/*
 * This defines the structures used for communication between modules in
 * the I2C network.
 */

#ifndef I2CMODULE_H
#define I2CMODULE_H

#include <Arduino.h>


/*
 * The basic message format
 */

typedef struct {
  byte start;        // Should always be I2CM_START
  byte message_type; // Code indicating the type of message being sent
  byte message_len;  // Total length of message excluding header
  byte data[0];
} I2CM_message_t;

#define I2CM_START 0x79

#define I2CM_TYPE_INTRO_REQUEST 0x01 // Message requesting intro from module
#define I2CM_TYPE_INTRO         0x02

// I2CM_TYPE_INTRO
typedef struct {
  byte address;
  char name[8];
} I2CM_intro_t;


class I2CM_Slave
{
  public:
  I2CM_Slave(char *name); // Initialize the slave's address from EEPROM
  I2CM_Slave(char *name, byte _address);

  private:
  I2CM_intro_t info;

  int address;
  char name[8];
};


#endif
