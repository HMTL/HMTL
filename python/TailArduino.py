#!/usr/bin/python
#
# This script reads from serial and prints the output
#

from __future__ import print_function

import argparse
import time
import serial
import threading

import hmtl.portscan as portscan
from hmtl.CircularBuffer import CircularBuffer


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


# Thread to read from the serial port and submit to a queue
def reader_thread(buff, options):
    print("Opening connection to '%s' at %d baud." %
          (options.device, options.baud))
    ser = serial.Serial(options.device, options.baud, timeout=10)

    while True:
        data = ser.readline()

        if data and len(data) > 0:
            buff.put((data, time.time()))


def main():
    options = handle_args()

    buff = CircularBuffer(1000)

    # Launch the reading thread
    reader = threading.Thread(target=reader_thread, args=(buff, options))
    reader.daemon = True  # Set to daemon so ctrl-C works
    reader.start()
    
    starttime = time.time()
    while True:
        # Wait for items to show up on the que
        item = buff.get()
        if not item:
            continue


        (data, ts) = item
        data = data.strip()
        if options.timestamp:
            # Add a beginning of line timestamp
            print("\033[91m[%.3f]\033[97m " % (ts - starttime), end="")

        try:
            # Attempt to print the item as ascii
            print("%s" % (data.decode()))
        except UnicodeDecodeError:
            # Failing that, print raw data
            print("'%s'" % data)

if __name__ == '__main__':
    main()