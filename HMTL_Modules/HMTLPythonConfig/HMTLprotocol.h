#ifndef HMTLPROTOCOL_H
#define HMTLPROTOCOL_H

#define HMTL_READY        "ready"
#define HMTL_ACK          "ok"
#define HMTL_CONFIG_START "start"
#define HMTL_CONFIG_END   "end"
#define HMTL_CONFIG_PRINT "print"

#define HMTL_TERMINATOR   (uint32_t)(0xFEFEFEFE)

#define CONFIG_START_BYTE 0xFD
#define CONFIG_START_SIZE 2

#endif
