#!/usr/bin/python
#
# This reads a HMTL configuration in JSON format and sends it over the serial
# connection to a HMTL module

import sys

from optparse import OptionParser

import hmtl.portscan as portscan
import hmtl.HMTLprotocol as HMTLprotocol
import hmtl.HMTLjson as HMTLjson
from hmtl.HMTLSerial import *


def handle_args():
    global options

    parser = OptionParser()
    parser.add_option("-f", "--file", dest="filename", 
                      help="HMTL configuration file", metavar="FILE")
    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")
    parser.add_option("-a", "--address", dest="address", type="int",
                      help="Set address")
    parser.add_option("-i", "--device_id", dest="device_id", type="int",
                      help="Set device_id")
    parser.add_option("-b", "--baud", dest="baud", type="int",
                      help="Set baud")

    parser.add_option("-w", "--write", dest="writeconfig", action="store_true",
                      help="Write configuration", default=False)
    parser.add_option("-p", "--print", dest="printconfig", action="store_true",
                      help="Read and print the current configuration", default=False)
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)

    (options, args) = parser.parse_args()

    # Required args
    if ((options.filename is None) and
        (options.printconfig is False) and
        (options.address is None) and
        (options.device_id is None) and
        (options.baud is None)):
        parser.print_help()
        exit("Must specify mode")

    if ((options.baud != None) and (options.baud % 1200 != 0)):
        exit("Baud must be a multiple of 1200")

    if (options.device == None):
        options.device = portscan.choose_port()

    if (options.device == None):
        parser.print_help()
        exit("Must specify device")

    if (options.printconfig):
        options.verbose = True

    return (options, args)

# Send the entire configuration
def send_configuration(config_data):
    print("***** Sending configuration *****")

    if (ser.send_command(HMTLprotocol.HMTL_CONFIG_START) == False):
        print("Failed to get ack from start message")
        exit(1)

    header_struct = HMTLprotocol.get_header_struct(config_data)
    ser.send_config('header', header_struct)

    for output in config_data["outputs"]:
        output_struct = HMTLprotocol.get_output_struct(output)
        ser.send_config(output["type"], output_struct)

    if (ser.send_command(HMTLprotocol.HMTL_CONFIG_END) == False):
        print("Failed to get ack from end message")
        exit(1)

# Just send commands to read the existing configuration and set the address
def send_address(address):
    print("***** Setting address to %d *****" % (address))

    ser.send_command(HMTLprotocol.HMTL_CONFIG_START)
    ser.send_config("address", HMTLprotocol.get_address_struct(address))
    ser.send_command(HMTLprotocol.HMTL_CONFIG_END)

# Just send commands to read the existing configuration and set the device_id
def send_device_id(device_id):
    print("***** Setting device_id to %d *****" % (device_id))

    ser.send_command(HMTLprotocol.HMTL_CONFIG_START)
    ser.send_config("device_id", HMTLprotocol.get_device_id_struct(device_id))
    ser.send_command(HMTLprotocol.HMTL_CONFIG_END)

# Just send commands to read the existing configuration and set the baud
def send_baud(baud):
    print("***** Setting baud to %d *****" % (baud))

    ser.send_command(HMTLprotocol.HMTL_CONFIG_START)
    ser.send_config("baud", HMTLprotocol.get_baud_struct(baud))
    ser.send_command(HMTLprotocol.HMTL_CONFIG_END)

def main():
    global ser

    handle_args()

    config_data = None
    if (options.filename != None):
        config_data = HMTLjson.load_json(options.filename)
        if (config_data == None):
            print("ERROR: Exiting due to invalid configuration file")
            exit(1)

        if (options.address != None):
            # Set the address in the read config
            print("* Setting address to %d" % options.address)
            config_data["header"]["address"] = options.address
        if (options.device_id != None):
            # Set the device_id in the read config
            print("* Setting device_id to %d" % options.device_id)
            config_data["header"]["device_id"] = options.device_id
        if (options.baud != None):
            # Set the baud in the read config
            print("* Setting baud to %d" % options.baud)
            config_data["header"]["baud"] = options.baud

    # Open the serial connection and wait for connection
    ser = HMTLSerial(options.device, 
                     verbose=options.verbose)

    if (options.printconfig == True):
        # Only read and print the configuration
        ser.send_command(HMTLprotocol.HMTL_CONFIG_READ)
        options.verbose = True
        ser.send_command(HMTLprotocol.HMTL_CONFIG_PRINT)
        exit(0)

    if (config_data != None):
        # Send the configuration
        send_configuration(config_data)

    else:

        # If not sending the entire configuration, first read the existing
        # configuration before modifying it.
        ser.send_command(HMTLprotocol.HMTL_CONFIG_READ)

        if (options.address != None):
            # Send the address update
            send_address(options.address)
        if (options.device_id != None):
            # Send the device_id update
            send_device_id(options.device_id)
        if (options.baud != None):
            # Send the baud update
            send_baud(options.baud)

    if (options.verbose):
        # Have the module output its entire configuration
        ser.send_command(HMTLprotocol.HMTL_CONFIG_PRINT)

    if (options.writeconfig == True):
        if (config_data):
            if ((config_data["header"]["address"] == 0) or
                (config_data["header"]["device_id"] == 0)):
                print("Both device ID and address must be set before write")
                sys.exit(1)

        # Write out the module's config to EEPROM
        ser.send_command(HMTLprotocol.HMTL_CONFIG_WRITE)

        print("Wrote config")
    else:
        print("Did not write config")

main()
