#
# Load and validate HMTL json configuration files
#

import json
from pprint import pprint
import HMTLprotocol

def load_json(filename):
    # Parse the JSON configuration file
    json_file = open(filename)
    config_data = json.load(json_file)
    json_file.close()

    print("****** Config read from '" + filename + "':")
    pprint(config_data);

    if (not validate_config(config_data)):
        print("Config validation failed")
        return None

    return config_data


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
    if (not output["type"] in HMTLprotocol.CONFIG_TYPES):
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
    elif (output["type"] == "xbee"):
        if (not check_required(output, "recvpin")): return False
        if (not check_required(output, "xmitpin")): return False
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

    print("***** Validating configuration data *****")

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
    if (not "device_id" in config):
        print("No device_id in config")
        return False
    if (not "baud" in config):
        print("No baud in config")
        return False
    if (not "flags" in config):
        config['flags'] = 0

    if (not "outputs" in data):
        print("Input file does not contain 'data'")
        return False

    for output in data["outputs"]:
        if (not validate_output(output)):
            return False;

    # Perform post-validation processing
    for output in data["outputs"]:
        post_process_config(output)

    return True
