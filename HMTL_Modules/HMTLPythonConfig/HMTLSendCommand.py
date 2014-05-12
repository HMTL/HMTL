#!/usr/bin/python
#
# This script sends HMTL formatted commands to the serial device
#

from optparse import OptionParser
import serial
import time

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
        ser.send_and_confirm(HMTLprotocol.get_test_struct(output, 255), False)
        time.sleep(1)

        print("Turning output %d off" % (output))
        ser.send_and_confirm(HMTLprotocol.get_test_struct(output, 0), False)
        output += 1

main()
