#!/usr/bin/python
##!/usr/local/bin/python3
#

from optparse import OptionParser
from server import *
from client import *


def handle_args():
    global options

    parser = OptionParser()
    parser.add_option("-s", "--server", dest="servermode", action="store_true",
                      help="Run as a server", default=False)
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

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
