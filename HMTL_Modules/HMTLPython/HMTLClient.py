#!/usr/bin/python
#
# Launch a client/server HMTL command system

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
from client import *

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-r", "--rgb", dest="setrgb", action="store_true",
                      help="Set RGB value", default=False)
    parser.add_option("-P", "--period", dest="period", type="float",
                      help="Sleep period between changes")
    parser.add_option("-A", "--hmtladdress", dest="hmtladdress", type="int",
                      help="Address to which messages are sent")

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

    return (options, args)
 
def main():

    handle_args()

    client = HMTLClient(options)
    client.test()
    client.close()

    print("Done.")
    exit(0)

main()
