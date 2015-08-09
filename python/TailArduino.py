#!/usr/bin/python
#
# This script reads from serial and prints the output
#

from __future__ import print_function

import argparse
import time

import hmtl.portscan as portscan
from hmtl.SerialBuffer import SerialBuffer

def handle_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--device", dest="device",
                        help="Arduino USB device")

    parser.add_argument("-b", "--baud", dest="baud", type=int,
                        help="Serial port baud ([9600], 19200, 57600, 115200)",
                        default=9600)
    
    parser.add_argument("-t", "--timestamp", dest="timestamp",
                        action="store_true",
                        help="Print timestamps", default=False)

    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true",
                        help="Verbose output", default=False)

    options = parser.parse_args()

    if options.device is None:
        options.device = portscan.choose_port()

    if options.device is None:
        parser.print_help()
        exit("Must specify device")

    return options


def main():
    options = handle_args()

    reader = SerialBuffer(options.device, options.baud)
    reader.start()

    buff = reader.get_buffer()

    start_time = time.time()
    while True:
        # Wait for items to show up on the que
        item = buff.get()
        if not item:
            continue

        data = item.data.strip()
        if options.timestamp:
            # Add a beginning of line timestamp
            print("\033[91m[%.3f]\033[97m " % (item.timestamp - start_time),
                  end="")

        try:
            # Attempt to print the item as ascii
            print("%s" % (data.decode()))
        except UnicodeDecodeError:
            # Failing that, print raw data
            print("'%s'" % data)

if __name__ == '__main__':
    main()