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
    "rs485"   : 0x6
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

#
# Configuration validation
#

def check_required(output, value, length=None):
    if (not value in output):
        print("ERROR: '" + value + "' is required in '" + output["type"] + "' config")
        return False
    if (length and (len(output[value]) != length)):
        print("ERROR: '%s' field should have length %d in '%s' config" %
              (value, length, output["type"]))
        return False

    return True

def validate_output(output):
    """Verify that an output's configuration is valid"""
    if (not "type" in output):
        print("No 'type' field in output: " + str(output))
        return False
    if (not output["type"] in CONFIG_TYPES):
        print(output["type"] + " is not a valid HMTL type")
        return False

    if (output["type"] == "value"):
        # There should be a pin and value fields
        if (not check_required(output, "pin")): return False
        if (not check_required(output, "value")): return False
    elif (output["type"] == "rgb"):
        # There should be three pins and three values
        if (not check_required(output, "pins", 3)): return False
        if (not check_required(output, "values", 3)): return False
    elif (output["type"] == "pixels"):
        if (not check_required(output, "clockpin")): return False
        if (not check_required(output, "datapin")): return False
        if (not check_required(output, "numpixels")): return False
        if (not check_required(output, "rgbtype")): return False
    elif (output["type"] == "rs485"):
        if (not check_required(output, "recvpin")): return False
        if (not check_required(output, "xmitpin")): return False
        if (not check_required(output, "enablepin")): return False
    elif (output["type"] == "mpr121"):
        if (not check_required(output, "irqpin")): return False
        if (not check_required(output, "useinterrupt")): return False
        if (not check_required(output, "trigger", 12)): return False
        if (not check_required(output, "release", 12)): return False
        for val in output["trigger"]:
            if (val > 0xF):
                print("ERROR: MPR121 trigger values must be <= %d" % (0xF))
                return False
        for val in output["release"]:
            if (val > 0xF):
                print("ERROR: MPR121 release values must be <= %d" % (0xF))
                return False


#XXX: Continue here

#XXX: Should check for any value matching HMTL_TERMINATION???

    return True

def post_process_config(output):
    if (output["type"] == "mpr121"):
        # Need to combine trigger and release values into single threshold
        # value:  (release << 4) | (trigger)
        output["threshold"] = [ 0 for i in range(0, len(output["trigger"]))]
        for i in range(0, len(output["trigger"])):
            output["threshold"][i] = (output["release"][i] << 4) | (output["trigger"][i])
        #print("thresholds:", [ "%x" % val for val in output["threshold"]])


def validate_config(data):
    """Verify that the configuration file is valid"""

    print("* Validating configuration data")

    if (not "header" in data):
        print("Input file does not contain 'header'")
        return False

    config = data["header"]
    if (not "protocol_version" in config):
        print("No protocol_version in config")
        return False
    if (not "hardware_version" in config):
        print("No hardware_version in config")
        return False
    if (not "address" in config):
        print("No address in config")
        return False
    if (not "flags" in config):
        config['flags'] = 0

    if (not "outputs" in data):
        print("Input file does not contain 'data'")
        return False

    for output in data["outputs"]:
        if (not validate_output(output)):
            return False;

    for output in data["outputs"]:
        post_process_config(output)

    return True

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
    print("get_output_struct: %s" % (output))
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
        print("TEST:", args)
        packed_output = struct.pack(*args)
    else:
        packed_output = b"" # XXX

    return packed_start + packed_hdr + packed_output;
