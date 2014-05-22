#!/usr/local/bin/python3
#
# Definitions of protocol for talking to HMTL modules via serial
#

"""HMTL Protocol definitions module"""

import struct

# Protocol commands
HMTL_CONFIG_READY  = "ready"
HMTL_CONFIG_ACK    = "ok"
HMTL_CONFIG_FAIL   = "fail"

HMTL_CONFIG_START  = "start"
HMTL_CONFIG_END    = "end"
HMTL_CONFIG_READ   = "read"
HMTL_CONFIG_PRINT  = "print"
HMTL_CONFIG_WRITE  = "write"

HMTL_TERMINATOR    = b'\xfe\xfe\xfe\xfe' # Indicates end of command

#
# Config starts with the config start byte, followed by the type of object,
# followed by the encoded form of that onbject
#
CONFIG_START_FMT = '<BB'
CONFIG_START_BYTE = 0xFD

# These values must match those in HMTLTypes.h
CONFIG_TYPES = {
    "header"  : 0x0,
    "value"   : 0x1,
    "rgb"     : 0x2,
    "program" : 0x3,
    "pixels"  : 0x4,
    "mpr121"  : 0x5,
    "rs485"   : 0x6,
    
    # The following values are for special commands
    "address" : 0xE0,
}

# Individial object formats
HEADER_FMT = '<BBBHBBB'
HEADER_MAGIC = 0x5C

OUTPUT_HDR_FMT   = '<BB'
OUTPUT_VALUE_FMT = '<Bh'
OUTPUT_RGB_FMT   = '<BBBBBB'
OUTPUT_PIXELS_FMT = '<BBHB'
OUTPUT_MPR121_FMT = '<BB' + 'B'*12
OUTPUT_RS485_FMT = '<BBB'

UPDATE_ADDRESS_FMT = '<H'

# HMTL Message formats
MSG_HDR_FMT = "<BBBBH"
MSG_VALUE_FMT = "<H"
MSG_RGB_FMT = "BBB"

MSG_BASE_LEN = 6 + 2
MSG_VALUE_LEN = MSG_BASE_LEN + 2
MSG_RGB_LEN = MSG_BASE_LEN + 3


BROADCAST = 65535


#
# Configuration formatting
#

def get_config_start(type):
    packed = struct.pack(CONFIG_START_FMT,
                         CONFIG_START_BYTE,
                         CONFIG_TYPES[type])
    return packed

def get_header_struct(data):
    config = data['header']

    packed_start = get_config_start('header')

    packed = struct.pack(HEADER_FMT,
                         HEADER_MAGIC,
                         config['protocol_version'],
                         config['hardware_version'],
                         config['address'],
                         0,
                         len(data['outputs']),
                         config['flags'])
    return packed_start + packed

def get_output_struct(output):
#    print("get_output_struct: %s" % (output))
    type = output["type"]

    packed_start = get_config_start(type)
    packed_hdr = struct.pack(OUTPUT_HDR_FMT,
                             CONFIG_TYPES[type], # type
                             0) # output # filled in by module
    if (type == "value"):
        packed_output = struct.pack(OUTPUT_VALUE_FMT,
                                    output["pin"],
                                    output["value"])
    elif (type == "rgb"):
        packed_output = struct.pack(OUTPUT_RGB_FMT,
                                    output['pins'][0],
                                    output['pins'][1],
                                    output['pins'][2],
                                    output['values'][0],
                                    output['values'][1],
                                    output['values'][2])
    elif (type == "pixels"):
        packed_output = struct.pack(OUTPUT_PIXELS_FMT,
                                    output['clockpin'],
                                    output['datapin'],
                                    output['numpixels'],
                                    output['rgbtype'])
    elif (type == "rs485"):
        packed_output = struct.pack(OUTPUT_RS485_FMT,
                                    output['recvpin'],
                                    output['xmitpin'],
                                    output['enablepin'])
    elif (type == "mpr121"):
        args = [OUTPUT_MPR121_FMT, output["irqpin"],
                output["useinterrupt"]] + [x for x in output["threshold"]]
        packed_output = struct.pack(*args)
    else:
        raise Exception("Unknown type %s" % (type))

    return packed_start + packed_hdr + packed_output

def get_address_struct(address):
    print("get_address_struct: address %d" % (address))

    packed_start = get_config_start("address")
    packed_address = struct.pack(UPDATE_ADDRESS_FMT,
                                 address)
    
    return packed_start + packed_address


#
# HMTL Message types
#

def get_msg_hdr(msglen, address):
    packed = struct.pack(MSG_HDR_FMT,
                         0xFC, # Startcode
                         0,    # CRC - XXX: TODO!
                         1,    # Protocol version
                         msglen,   # Message length
                         address)  # Destination address 65535 is "Any"
    return packed

def get_output_hdr(type, output):
    packed = struct.pack(OUTPUT_HDR_FMT,
                         CONFIG_TYPES[type], # Message type 
                         output)             # Output number
    return packed

def get_value_msg(address, output, value):
    packed_hdr = get_msg_hdr(MSG_VALUE_LEN, address)
    packed_out = get_output_hdr("value", output)
    packed = struct.pack(MSG_VALUE_FMT,
                         value)                 # Value to set

    return packed_hdr + packed_out + packed                        

def get_rgb_msg(address, output, r, g, b):
    packed_hdr = get_msg_hdr(MSG_RGB_LEN, address)
    packed_out = get_output_hdr("rgb", output)
    packed = struct.pack(MSG_RGB_FMT, r, g, b)

    return packed_hdr + packed_out + packed
