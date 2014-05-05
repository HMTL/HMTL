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

/* Terminator indicating that a complete command has been received */
#define HMTL_TERMINATOR   (uint32_t)(0xFEFEFEFE)

#define CONFIG_START_BYTE 0xFD // Beginning of a binary comand
#define CONFIG_START_SIZE 2    // Length of a binary command

#endif
