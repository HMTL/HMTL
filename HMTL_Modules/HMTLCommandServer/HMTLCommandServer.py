#!/usr/bin/python
##!/usr/local/bin/python3
#
# Launch a client/server HMTL command system

import sys
sys.path.append("/Users/amp/Dropbox/Arduino/HMTL/HMTL_Modules/HMTLPython")

from optparse import OptionParser
from server import *
from client import *
import portscan

def handle_args():
    global options

    parser = OptionParser()

    # Server options
    parser.add_option("-s", "--server", dest="servermode", action="store_true",
                      help="Run as a server", default=False)
    parser.add_option("-d", "--device", dest="device",
                      help="Arduino USB device")

    # General options
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")


    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))



    if (options.device == None):
        options.device = portscan.choose_port()

    # Required args
    if (options.servermode):
        pass
    else:
        pass
    
    return (options, args)
 
def main():

    handle_args()

    if (options.servermode):
        server = HMTLServer(options)
        server.start()
        server.wait()
    else:
        client = HMTLClient(options)
        client.start()
        client.wait()


    print("Done.")
    exit(0)

    

main()
