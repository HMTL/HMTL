#!/usr/bin/python
#
# Launch a HMTL command server

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
from server import *
from client import *
import portscan

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")
    parser.add_option("-n", "--dryrun", dest="dryrun", action="store_true",
                      help="Perform dryrun only", default=False)
    parser.add_option("-b", "--baud", dest="baud", type="int",
                      help="Serial port baud ([9600], 19200, 57600, 115200)",
                      default=9600)

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")


    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))



    if ((options.device == None) and (not options.dryrun)):
        options.device = portscan.choose_port()
    
    return (options, args)
 
def main():

    handle_args()

    server = HMTLServer(options)
    server.listen()
    server.close()

    print("Done.")
    exit(0)

main()

