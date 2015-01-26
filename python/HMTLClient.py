#!/usr/bin/python
#
# Launch a client/server HMTL command system

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from optparse import OptionParser, OptionGroup
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
                      help="Address to which messages are sent [default=BROADCAST]", 
                      default=HMTLprotocol.BROADCAST)

    # Command types
    group = OptionGroup(parser, "Command Types")
    group.add_option("-V", "--value", action="store_const",
                      dest="commandtype", const="value",
                      help="Send value command", default=None)
    group.add_option("-R", "--rgb", action="store_const",
                      dest="commandtype", const="rgb",
                      help="Send rgb command")
    group.add_option("-B", "--blink", action="store_const",
                      dest="commandtype", const="blink",
                      help="Send blink program")
    group.add_option("-T", "--timedchange", action="store_const",
                      dest="commandtype", const="timedchange",
                      help="Send timed change program")
    group.add_option("-N", "--none", action="store_const",
                      dest="commandtype", const="none",
                      help="Send program reset command")
    group.add_option("-P", "--poll", action="store_const",
                      dest="commandtype", const="poll",
                      help="Send module polling command")
    group.add_option("-S", "--setaddr", action="store_const",
                      dest="commandtype", const="setaddr",
                      help="Send address setting command")
    parser.add_option_group(group)


    # Command options
    group = OptionGroup(parser, "Command Options")
    group.add_option("--period", dest="period", type="float",
                      help="Sleep period between changes")
    group.add_option("-O", "--output", dest="output", type="int",
                      help="Number of the output to be set", default=None)
    group.add_option("-C", "--command", dest="commandvalue", action="store",
                      default = None)
    parser.add_option_group(group)

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

    # Need some argument validation
    if (options.commandtype == None):
        pass
    elif (options.commandtype == "poll"):
        pass
    elif (options.commandtype == "setaddr"):
        pass
    elif (options.commandtype == "none"):
        pass
    else:
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

    msg = None
    expect_response = False

    if (options.commandtype == "value"):
        print("Sending value message.  Address=%d Output=%d Value=%d" %
              (options.hmtladdress, options.output, int(options.commandvalue)))
        msg = HMTLprotocol.get_value_msg(options.hmtladdress,
                                         options.output,
                                         int(options.commandvalue))
    elif (options.commandtype == "rgb"):
        (r,g,b) = options.commandvalue.split(",")
        print("Sending RGB message.  Address=%d Output=%d Value=%d,%d,%d" %
              (options.hmtladdress, options.output, int(r), int(g), int(b)))
        msg = HMTLprotocol.get_rgb_msg(options.hmtladdress,
                                       options.output,
                                       int(r), int(g), int(b))

    elif (options.commandtype == "blink"):
        (a,b,c,d, e,f,g,h) = options.commandvalue.split(",")
        print("Sending BLINK message. Address=%d Output=%d on_period=%d on_value=%s off_period=%d off_values=%s" % (options.hmtladdress, options.output, int(a), [int(b),int(c),int(d)],
                                         int(e), [int(f),int(g),int(h)]))
        msg = HMTLprotocol.get_program_blink_msg(options.hmtladdress,
                                         options.output,
                                         int(a), [int(b),int(c),int(d)],
                                         int(e), [int(f),int(g),int(h)])
    elif (options.commandtype == "timedchange"):
        (a, b,c,d, e,f,g,) = options.commandvalue.split(",")
        print("Sending TIMED CHANGE message. Address=%d Output=%d change_period=%d start_value=%s stop_values=%s" % (options.hmtladdress, options.output, int(a), [int(b),int(c),int(d)],
                                                                                                          [int(e),int(f),int(g)]))
        msg = HMTLprotocol.get_program_timed_change_msg(options.hmtladdress,
                                        options.output,
                                        int(a),
                                        [int(b),int(c),int(d)],
                                        [int(e),int(f),int(g)])


    elif (options.commandtype == "none"):
        print("Sending NONE message.  Output=%d" % (options.output))
        msg = HMTLprotocol.get_program_none_msg(options.hmtladdress,
                                                options.output)
    elif (options.commandtype == "poll"):
        print("Sending poll message.  Address=%d" %
              (options.hmtladdress))
        msg = HMTLprotocol.get_poll_msg(options.hmtladdress)
        expect_response = True
    elif (options.commandtype == "setaddr"):
        (device_id, new_address) = options.commandvalue.split(",")
        print("Sending set address message.  Address=%d Device=%d NewAddress=%d" %
              (options.hmtladdress, int(device_id), int(new_address)))
        msg = HMTLprotocol.get_set_addr_msg(options.hmtladdress, 
                                            int(device_id), int(new_address))

    if (msg != None):
        starttime = time.time()
        client.send_and_ack(msg, expect_response)
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
