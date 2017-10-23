################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2017
#
# Constant definitions for module communications
#
################################################################################

################################################################################
#
# Constants
#
################################################################################

#
# Config starts with the config start byte, followed by the type of object,
# followed by the encoded form of that object
#
CONFIG_START_FMT = '<BB'
CONFIG_START_BYTE = 0xFD

# These values must match those in HMTLTypes.h
CONFIG_TYPES = {
    "header": 0x0,
    "value": 0x1,
    "rgb": 0x2,
    "program": 0x3,
    "pixels": 0x4,
    "mpr121": 0x5,
    "rs485": 0x6,
    "xbee": 0x7,

    # The following values are for special commands
    "address": 0xE0,
    "device_id": 0xE1,
    "baud": 0xE2,
}

# Individial object formats
HEADER_FMT = '<BBBBBBHH'
HEADER_MAGIC = 0x5C

OUTPUT_HDR_FMT = '<BB'
OUTPUT_VALUE_FMT = '<Bh'
OUTPUT_RGB_FMT = '<BBBBBB'
OUTPUT_PIXELS_FMT = '<BBHB'
OUTPUT_MPR121_FMT = '<BB' + 'B' * 12
OUTPUT_RS485_FMT = '<BBB'
OUTPUT_XBEE_FMT = '<BB'

OUTPUT_ALL_OUTPUTS = 254

UPDATE_ADDRESS_FMT = '<H'
UPDATE_DEVICE_ID_FMT = '<H'
UPDATE_BAUD_FMT = '<B'

#
# Utility
#


def baud_to_byte(baud):
    return baud / 1200


def byte_to_baud(data):
    return data * 1200
