################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2017
#
# Class definitions and functions for communicating configuration data to
# HMTL modules.
#
################################################################################

"""HMTL Configuration definitions"""

import struct
from constants import *

# Configuration commands
CONFIG_START  = "start"
CONFIG_END    = "end"
CONFIG_READ   = "read"
CONFIG_PRINT  = "print"
CONFIG_WRITE  = "write"


################################################################################
#
# Legacy configuration methods
#
################################################################################

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
                             CONFIG_TYPES[type],  # type
                             0)  # output # filled in by module
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
    elif (type == "xbee"):
        packed_output = struct.pack(OUTPUT_XBEE_FMT,
                                    output['recvpin'],
                                    output['xmitpin'])

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

################################################################################
#
# Configuration object classes
#
################################################################################


class BaseConfig(object):
    @classmethod
    def from_data(cls, data, offset=0):
        header = struct.unpack_from(cls.FORMAT, data, offset)
        return cls(*header)

    @classmethod
    def type(cls):
        return cls.TYPE


class ConfigHeaderMain(BaseConfig):
    """This class is for the config_hdr_t structure"""
    TYPE = "CONFIG"
    FORMAT = HEADER_FMT

    def __init__(self, magic, protocol_version, hardware_version, baud,
                 num_outputs, flags, device_id, address):
        self.magic = magic;
        self.protocol_version = protocol_version
        self.hardware_version = hardware_version
        self.baud = baud
        self.num_outputs = num_outputs
        self.flags = flags
        self.device_id = device_id
        self.address = address

    def __str__(self):
        return """  config_hdr_t:
    magic:%02x
    proto_ver:%d
    hdw_ver:%d
    baud:%d
    outputs:%d
    flags:%02x
    dev_id:%d
    addr:%d
""" % (
            self.magic,
            self.protocol_version,
            self.hardware_version,
            byte_to_baud(self.baud),
            self.num_outputs,
            self.flags,
            self.device_id,
            self.address
        )

    def short(self):
        return "conf id:%d,addr:%d" % (self.device_id, self.address)


class ConfigHeaderValue(BaseConfig):
    TYPE = "Value"
    FORMAT = OUTPUT_VALUE_FMT
    LENGTH = 2

    def __init__(self, pin, value):
        self.output_hdr = None
        self.pin = pin
        self.value = value

    def __str__(self):
        return str(self.output_hdr) + """  config_value_t:
    pin:%s
    value:%s
        """ % (self.pin, self.value)

    def short(self):
        return "value pin:%d,val:%d" % (self.pin, self.value)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.pin, self.value)


class ConfigHeaderRGB(BaseConfig):
    TYPE = "RGB"
    FORMAT = OUTPUT_RGB_FMT
    LENGTH = 6

    def __init__(self,
                 pin_red, pin_green, pin_blue,
                 value_red, value_green, value_blue):
        self.output_hdr = None
        self.pins = [pin_red, pin_green, pin_blue]
        self.value = [value_red, value_green, value_blue]

    def __str__(self):
        return str(self.output_hdr) + """  config_rgb_t:
    pins:%s
    value:%s
        """ % (self.pins, self.value)

    def short(self):
        return "rgb pins:%s,val:%s" % (self.pins, self.value)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.pins[0], self.pins[1], self.pins[2],
                           self.value[0], self.value[1], self.value[2])


class ConfigHeaderPixels(BaseConfig):
    TYPE = "PIXELS"
    FORMAT = OUTPUT_PIXELS_FMT
    LENGTH = 5

    def __init__(self, clockpin, datapin, numpixels, rgbtype):
        self.output_hdr = None
        self.clockpin = clockpin
        self.datapin = datapin
        self.numpixels = numpixels
        self.rgbtype = rgbtype

    def __str__(self):
        return str(self.output_hdr) + """  config_pixels_t:
    clockpin:%d
    datapin:%d
    numpixels:%d
    rgbtype:%d
        """ % (self.clockpin, self.datapin, self.numpixels, self.rgbtype)

    def short(self):
        return "pixels dat:%d,clk:%d,num:%d" % (
            self.datapin, self.clockpin, self.numpixels)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.clockpin, self.datapin,
                           self.numpixels, self.rgbtype)


class ConfigHeaderRS485(BaseConfig):
    TYPE = "RS485"
    FORMAT = OUTPUT_RS485_FMT
    LENGTH = 3

    def __init__(self, recvpin, xmitpin, enablepin):
        self.output_hdr = None
        self.recvpin = recvpin
        self.xmitpin = xmitpin
        self.enablepin = enablepin

    def __str__(self):
        return str(self.output_hdr) + """  config_rs485_t:
    recvpin:%d
    xmitpin:%d
    enablepin:%d
        """ % (self.recvpin, self.xmitpin, self.enablepin)

    def short(self):
        return "rs485 recv:%d,xmit:%d,ena:%d" % (
            self.recvpin, self.xmitpin, self.enablepin)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.recvpin, self.xmitpin,
                           self.enablepin)


class ConfigHeaderXbee(BaseConfig):
    TYPE = "XBEE"
    FORMAT = OUTPUT_XBEE_FMT
    LENGTH = 2

    def __init__(self, recvpin, xmitvalue):
        self.output_hdr = None
        self.recvpin = recvpin
        self.xmitvalue = xmitvalue

    def __str__(self):
        return str(self.output_hdr) + """  config_xbee_t:
    recvpin:%s
    xmitvalue:%s
        """ % (self.recvpin, self.xmitvalue)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.recvpin, self.xmitvalue)


class ConfigHeaderMPR121(BaseConfig):
    TYPE = "MPR121"
    FORMAT = OUTPUT_MPR121_FMT
    LENGTH = 14

    def __init__(self, irqpin, useinterrupt, *thresholds):
        self.output_hdr = None
        self.irqpin = irqpin
        self.useinterrupt = useinterrupt
        self.thresholds = thresholds

    def __str__(self):
        return str(self.output_hdr) + """  config_mpr121_t:
    irqpin:%s
    useinterrupt:%s
    thresholds:%s
        """ % (self.irqpin, self.useinterrupt, self.thresholds)

    def short(self):
        return "mpr121 irq:%d,int:%s" % (self.irqpin, self.useinterrupt)

    def pack(self):
        return self.output_hdr.pack() + \
               struct.pack(self.FORMAT, self.irqpin, self.useinterrupt,
                           *self.thresholds)


################################################################################
#
# Helper functions
#
################################################################################

def config_types(headers):
    types = []
    for hdr in headers:
        if not isinstance(hdr, ConfigHeaderMain):
            types.append(hdr.type())
    return types