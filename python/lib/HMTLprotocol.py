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
    "address"   : 0xE0,
    "device_id" : 0xE1,
    "baud"      : 0xE2,
    }

# Individial object formats
HEADER_FMT = '<BBBBBBHH'
HEADER_MAGIC = 0x5C

OUTPUT_HDR_FMT   = '<BB'
OUTPUT_VALUE_FMT = '<Bh'
OUTPUT_RGB_FMT   = '<BBBBBB'
OUTPUT_PIXELS_FMT = '<BBHB'
OUTPUT_MPR121_FMT = '<BB' + 'B'*12
OUTPUT_RS485_FMT = '<BBB'

UPDATE_ADDRESS_FMT = '<H'
UPDATE_DEVICE_ID_FMT = '<H'
UPDATE_BAUD_FMT = '<B'

# HMTL Message formats
MSG_HDR_FMT = "<BBBBH" # All HMTL messages start with this
MSG_VALUE_FMT = "H"
MSG_RGB_FMT = "BBB"
MSG_PROGRAM_FMT = "B"

MSG_BASE_LEN = 6 + 2
MSG_VALUE_LEN = MSG_BASE_LEN + 2
MSG_RGB_LEN = MSG_BASE_LEN + 3

MSG_PROGRAM_VALUE_LEN = 12
MSG_PROGRAM_LEN = MSG_BASE_LEN + 1 + MSG_PROGRAM_VALUE_LEN

BROADCAST = 65535

# Specific program formats
MSG_PROGRAM_NONE_TYPE = 0
MSG_PROGRAM_NONE_FMT = 'B'*MSG_PROGRAM_VALUE_LEN

MSG_PROGRAM_BLINK_TYPE = 1
MSG_PROGRAM_BLINK_FMT = '<HBBBHBBB' + 'BB' # Msg + padding

MSG_PROGRAM_TIMED_CHANGE_TYPE = 2
MSG_PROGRAM_TIMED_CHANGE_FMT = '<LBBBBBB' + 'BB' # Msg + padding

#
# Utility
#
def baud_to_byte(baud):
    return (baud / 1200)

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
                         baud_to_byte(config['baud']),
                         len(data['outputs']),
                         config['flags'],
                         config['device_id'],
                         config['address'])

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

def get_device_id_struct(device_id):
    print("get_device_id_struct: device_id %d" % (device_id))

    packed_start = get_config_start("device_id")
    packed_device_id = struct.pack(UPDATE_DEVICE_ID_FMT,
                                 device_id)
    
    return packed_start + packed_device_id

def get_baud_struct(baud):
    print("get_baud_struct: baud %d" % (baud))

    packed_start = get_config_start("baud")
    packed_baud = struct.pack(UPDATE_BAUD_FMT,
                              baud_to_byte(baud))
    
    return packed_start + packed_baud


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

def get_program_msg(address, output, program_type, program_data):
    if (len(program_data) != MSG_PROGRAM_VALUE_LEN):
        raise Exception("Program data must be %d bytes" % (MSG_PROGRAM_VALUE_LEN))

    packed_hdr = get_msg_hdr(MSG_PROGRAM_LEN, address)
    packed_out = get_output_hdr("program", output)
    packed = struct.pack(MSG_PROGRAM_FMT, program_type)

    return packed_hdr + packed_out + packed + program_data

def get_program_blink_msg(address, output, 
                          on_period, on_values, off_period, off_values):
    blink = struct.pack(MSG_PROGRAM_BLINK_FMT,
                        on_period, on_values[0], on_values[1], on_values[2],
                        off_period, off_values[0], off_values[1], off_values[2],
                        0, 0)
    return get_program_msg(address, output, MSG_PROGRAM_BLINK_TYPE, blink)
    
def get_program_none_msg(address, output):
    args = [MSG_PROGRAM_NONE_FMT] + [0 for x in range(0, MSG_PROGRAM_VALUE_LEN)]
    msg = struct.pack(*args)
    return get_program_msg(address, output, MSG_PROGRAM_NONE_TYPE, msg)

def get_program_timed_change_msg(address, output,
                                 change_period, start_values, stop_values):
    msg = struct.pack(MSG_PROGRAM_TIMED_CHANGE_FMT,
                      change_period,
                      start_values[0], start_values[1], start_values[2],
                      stop_values[0], stop_values[1],stop_values[2],
                      0, 0)
    return get_program_msg(address, output, MSG_PROGRAM_TIMED_CHANGE_TYPE, msg)
