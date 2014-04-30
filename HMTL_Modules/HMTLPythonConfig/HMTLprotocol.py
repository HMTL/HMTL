#!/usr/local/bin/python3
#
# Definitions of protocol for talking to HMTL modules via serial
#

"""HMTL Protocol definitions module"""

# Protocol constants
HMTL_CONFIG_ACK=b"ok"

HMTL_CONFIG_START=b"start"
HMTL_CONFIG_END=b"end"

# These values must match those in HMTLTypes.h
types = {
    "value"   : 1,
    "rgb"     : 2,
    "program" : 3,
    "pixels"  : 4,
    "mpr121"  : 5,
    "rs485"   : 6
}

def check_required(output, value):
    if (not value in output):
        print("'" + value + "' is required in '" + output["type"] + "' config")
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
        if (not check_required(output, "pin")): return False;
XXX
        if (not "pin" in output):
            print("'pin' is required in 'value' config")
            return False
        if (not "value" in output):
            print("'value' is required in 'value' config")
            return False
    elif (output["type"] == "rgb"):
        # There should be three pins and three values
        if (not "pins" in output):
            print("'pins' is required in 'rgb' config")
            return False
        if (len(output["pins"]) != 3):
            print("'pins' field must be length 3 in 'rgb' config")
            return False
        if (not "values" in output):
            print("'values' is required in 'rgb' config")
            return False
        if (len(output["values"]) != 3):
            print("'values' field must be length 3 in 'rgb' config")
            return False
    elif (output["type"] == "pixels"):
        if (not "clockpin" in output):
            return False


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
