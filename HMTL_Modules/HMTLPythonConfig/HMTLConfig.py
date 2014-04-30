#!/usr/local/bin/python3
#
# This reads a HMTL configuration in JSON format and sends it over the serial
# connection to a HMTL module

from optparse import OptionParser
import json
from pprint import pprint
import serial
import HMTLprotocol

# XXX: The serial device for the Arduino.  This should be determined via
# scanning
device = '/dev/tty.usbserial-AM01SJJQ'

def handle_args():
    global options

    parser = OptionParser()
    parser.add_option("-f", "--file", dest="filename", 
                      help="HMTL configuration file", metavar="FILE")
    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")
    parser.add_option("-n", "--dryrun", dest="dryrun", action="store_true",
                      help="Perform dryrun only", default=False)

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

    # Required args
    if (options.filename == None):
        parser.print_help()
        exit("Must specify HMTL configuration file")

    return (options, args)

def get_line():
    data = ser.readline().strip().decode()
    print("get_line: received '%s'" % (data))
    return data

def send_command(command):
    print("send_command: sending '%s'" % (command))
    data = command + "\n"
    ser.write(bytes(data, 'utf-8'))

def waitForReady():
    """Wait for the Arduino to send its ready signal"""
    while True:
        data = get_line()
        if (len(data) == 0):
            print("Receive returned empty, timed out");
            return False
        if (data == HMTLprotocol.HMTL_CONFIG_READY):
            print("Recieved ready from Arduino")
            return True

def sendAndConfirm(data):
    """Send a command and wait for the ACK"""

    if (options.dryrun):
        return True

    send_command(data)
    ack = get_line()
    if (ack == HMTLprotocol.HMTL_CONFIG_ACK):
        return True
    else:
        return False

def sendConfig(data):
    if (sendAndConfirm(HMTLprotocol.HMTL_CONFIG_START) == False):
        print("Failed to get ack from start message")
        exit(1)

    print("Sending configuration");

    # XXX: Send the configuration

    if (sendAndConfirm(HMTLprotocol.HMTL_CONFIG_END) == False):
        print("Failed to get ack from end message")
        exit(1)


def main():
    global device
    global ser

    handle_args()

    if (options.device != None):
        device = options.device

    if (options.dryrun == False):
        # Open the serial connection and wait for 
        ser = serial.Serial(device, 9600, timeout=10)
        if (waitForReady() == False):
            exit(1)

    json_file = open(options.filename)
    data = json.load(json_file)
    json_file.close()
    
    print("* Config read from '" + options.filename + "':")
    pprint(data);

    if (not HMTLprotocol.validate_config(data)):
        print("Exiting due to invalid configuration file")
        exit(1)

    sendConfig(data)

main()
