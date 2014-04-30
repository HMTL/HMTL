#!/usr/local/bin/python3
#
# Definitions of protocol for talking to HMTL modules via serial
#

"""HMTL Protocol definitions module"""

# Protocol constants
HMTL_CONFIG_READY="Ready"

HMTL_CONFIG_ACK="ok"

HMTL_CONFIG_START="start"
HMTL_CONFIG_END="end"

# These values must match those in HMTLTypes.h
types = {
    "value"   : 1,
    "rgb"     : 2,
    "program" : 3,
    "pixels"  : 4,
    "mpr121"  : 5,
    "rs485"   : 6
}

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
    if (not output["type"] in types):
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

#XXX: Continue here
    return True

def validate_config(data):
    """Verify that the configuration file is valid"""

    print("* Validating configuration data")

    if (not "config" in data):
        print("Input file does not contain 'config'")
        return False

    if (not "outputs" in data):
        print("Input file does not contain 'data'")
        return False

    for output in data["outputs"]:
        if (not validate_output(output)):
            return False;

    return True
