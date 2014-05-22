#!/usr/bin/python
#
# This script reads from serial and prints the output
#
import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
import serial
import portscan

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
    global device
    global ser

    handle_args()

    if (options.device != None):
        device = options.device
    
    ser = serial.Serial(device, 9600, timeout=10)
    while True:
        data = ser.readline().strip()
        try:
            print("%s" % (data.decode()))
        except UnicodeDecodeError:
            print("'%s'" % (data))

main()
