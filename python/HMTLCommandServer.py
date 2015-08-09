#!/usr/bin/python
#
# Launch a HMTL command server

from optparse import OptionParser
from hmtl.server import *
import hmtl.portscan as portscan

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")
    parser.add_option("-b", "--baud", dest="baud", type="int",
                      help="Serial port baud ([9600], 19200, 57600, 115200)",
                      default=9600)

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="0.0.0.0")

    parser.add_option("-t", "--timeout", dest="timeout", type="int",
                      help="Timeout on serial", default=5)

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

