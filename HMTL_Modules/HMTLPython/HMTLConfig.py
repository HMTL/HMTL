#!/usr/bin/python
#
# This reads a HMTL configuration in JSON format and sends it over the serial
# connection to a HMTL module

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
import serial
from binascii import hexlify
import time

import portscan
import HMTLprotocol
import HMTLjson
from HMTLSerial import *


def handle_args():
    global options

    parser = OptionParser()
    parser.add_option("-f", "--file", dest="filename", 
                      help="HMTL configuration file", metavar="FILE")
    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")
    parser.add_option("-a", "--address", dest="address", type="int",
                      help="Set address");

    parser.add_option("-n", "--dryrun", dest="dryrun", action="store_true",
                      help="Perform dryrun only", default=False)
    parser.add_option("-w", "--write", dest="writeconfig", action="store_true",
                      help="Write configuration", default=False)
    parser.add_option("-p", "--print", dest="printconfig", action="store_true",
                      help="Read and print the current configuration", default=False)
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)



    (options, args) = parser.parse_args()
    # print("options:" + str(options) + " args:" + str(args))

    # Required args
    if ((options.filename == None) and
        (options.printconfig == False) and
        (options.address == None)):
        parser.print_help()
        exit("Must specify mode")

    if (options.device == None):
        options.device = portscan.choose_port()

    if ((options.dryrun == False) and (options.device == None)):
        parser.print_help()
        exit("Must specify device if not in dry-run mode");

    if (options.printconfig):
        options.verbose = True

    return (options, args)

# Send the entire configuration
def send_configuration(config_data):
    print("***** Sending configuration *****");

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

    ser.send_command(HMTLprotocol.HMTL_CONFIG_READ)
    ser.send_command(HMTLprotocol.HMTL_CONFIG_START)
    ser.send_config("address", HMTLprotocol.get_address_struct(address))
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


    # Open the serial connection and wait for connection
    ser = HMTLSerial(options.device, 
                     verbose=options.verbose, 
                     dryrun=options.dryrun)

    if (options.printconfig == True):
        # Only read and print the configuration
        ser.send_command(HMTLprotocol.HMTL_CONFIG_READ)
        options.verbose = True
        ser.send_command(HMTLprotocol.HMTL_CONFIG_PRINT)
        exit(0)

    if (config_data != None):
        # Send the configuration
        send_configuration(config_data)
    elif (options.address != None):
        # Send the address update
        send_address(options.address)

    if (options.verbose):
        # Have the module output its entire configuration
        ser.send_command(HMTLprotocol.HMTL_CONFIG_PRINT)

    if (options.writeconfig == True):
        # Write out the module's config to EEPROM
        ser.send_command(HMTLprotocol.HMTL_CONFIG_WRITE)
    else:
        print("Did not write config")

main()
