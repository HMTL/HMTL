/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * Protocol for communating with modules over a serial connection
 ******************************************************************************/

#ifndef HMTLPROTOCOL_H
#define HMTLPROTOCOL_H

/* Text commands */
#define HMTL_READY        "ready"
#define HMTL_ACK          "ok"
#define HMTL_FAIL         "fail"
#define HMTL_CONFIG_START "start"
#define HMTL_CONFIG_END   "end"
#define HMTL_CONFIG_READ  "read"
#define HMTL_CONFIG_PRINT "print"
#define HMTL_CONFIG_WRITE "write"

#define HMTL_COMMAND_ADDRESS   0xE0
#define HMTL_COMMAND_DEVICE_ID 0xE1
#define HMTL_COMMAND_BAUD      0xE2

/* Terminator indicating that a complete command has been received */
#define HMTL_TERMINATOR   (uint32_t)(0xFEFEFEFE)

#define CONFIG_START_BYTE 0xFD // Beginning of a binary comand
#define CONFIG_START_SIZE 2    // Length of a binary command

#endif
