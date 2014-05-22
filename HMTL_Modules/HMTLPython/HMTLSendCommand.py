#!/usr/bin/python
#
# This script sends HMTL formatted commands to the serial device
#

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
import serial
import time
from binascii import hexlify

import portscan
import HMTLprotocol
from HMTLSerial import *


def handle_args():
    global options

    parser = OptionParser()
    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)

    (options, args) = parser.parse_args()

    if (options.device == None):
        options.device = portscan.choose_port()

    if (options.device == None):
	    parser.print_help()
            exit("Must specify device");

    return (options, args)

def main():
    global ser

    handle_args()

    ser = HMTLSerial(options.device, verbose=options.verbose)

    output = 0
    while True:
        print("Turning output %d on" % (output))

        command = HMTLprotocol.get_value_msg(HMTLprotocol.BROADCAST, output, 255)
        print("  sending: %s" % (hexlify(command)))
        ser.send_and_confirm(command, False)
        time.sleep(1)

        print("Turning output %d off" % (output))
        command = HMTLprotocol.get_value_msg(HMTLprotocol.BROADCAST, output, 0)
        print("  sending: %s" % (hexlify(command)))
        ser.send_and_confirm(command, False)
        output = (output + 1) % 4

main()
