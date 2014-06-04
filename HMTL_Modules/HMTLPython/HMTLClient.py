#!/usr/bin/python
#
# Launch a client/server HMTL command system

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser
from client import *

import HMTLprotocol

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")
    parser.add_option("-k", "--killserver", dest="killserver", action="store_true",
                      help="Send a kill command to the server", default=False)

    # General options
    parser.add_option("-A", "--hmtladdress", dest="hmtladdress", type="int",
                      help="Address to which messages are sent", 
                      default=HMTLprotocol.BROADCAST)

    # Test mode
    parser.add_option("-t", "--testmode", dest="testmode", action="store_true",
                      help="Run test mode", default=False)
    parser.add_option("-r", "--rgbtest", dest="setrgb", action="store_true",
                      help="Set RGB value", default=False)

    # Command types
    parser.add_option("-V", "--value", action="store_const",
                      dest="commandtype", const="value",
                      help="Send value command", default=None)
    parser.add_option("-R", "--rgb", action="store_const",
                      dest="commandtype", const="rgb",
                      help="Send rgb command")
    parser.add_option("-B", "--blink", action="store_const",
                      dest="commandtype", const="blink",
                      help="Send blink command")
    parser.add_option("-N", "--none", action="store_const",
                      dest="commandtype", const="none",
                      help="Send program reset command")


    # Command options
    parser.add_option("-P", "--period", dest="period", type="float",
                      help="Sleep period between changes")
    parser.add_option("-O", "--output", dest="output", type="int",
                      help="Number of the output to be set", default=None)
    parser.add_option("-C", "--command", dest="commandvalue", action="store",
                      default = None)

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

    if (options.commandtype != None):
        # Need some argument validation
        if (options.output == None):
            print("Must specify an outout number")
            sys.exit(1)
        if (options.commandvalue == None):
            print("Must specify a command value")
            sys.exit(1)

    return (options, args)
 
def main():

    handle_args()

    client = HMTLClient(options)

    if (options.testmode):
        client.test()
    else:
        msg = None

        if (options.commandtype == "value"):
            print("Sending value message.  Output=%d Value=%d" %
                  (options.output, int(options.commandvalue)))
            msg = HMTLprotocol.get_value_msg(options.hmtladdress,
                                             options.output,
                                             int(options.commandvalue))
        elif (options.commandtype == "rgb"):
            (r,g,b) = options.commandvalue.split(",")
            print("Sending RGB message.  Output=%d Value=%d,%d,%d" %
                  (options.output, int(r), int(g), int(b)))
            msg = HMTLprotocol.get_rgb_msg(options.hmtladdress,
                                           options.output,
                                           int(r), int(g), int(b))
        
        elif (options.commandtype == "blink"):
            (a,b,c,d, e,f,g,h) = options.commandvalue.split(",")
            print("Sending BLINK message. Output=%d on_period=%d on_value=%s off_period=%d off_values=%s" % (options.output, int(a), [int(b),int(c),int(d)],
                                             int(e), [int(f),int(g),int(h)]))
            msg = HMTLprotocol.get_program_blink_msg(options.hmtladdress,
                                             options.output,
                                             int(a), [int(b),int(c),int(d)],
                                             int(e), [int(f),int(g),int(h)])
        elif (options.commandtype == "none"):
            print("Sending NONE message.  Output=%d" % (options.output))
            msg = HMTLprotocol.get_program_none_msg(options.hmtladdress,
                                                    options.output)
        

        if (msg != None):
            starttime = time.time()
            client.send_and_ack(msg)
            endtime = time.time()
            print("Sent and acked in %.6fs" % (endtime - starttime))

    if (options.killserver):
        # Send an exit message to the server
        print("Sending EXIT message to server")
        client.send_exit()

    client.close()

    print("Done.")
    exit(0)

main()
