#!/usr/bin/python
#
# Launch a client/server HMTL command system

import time

import hmtl.HMTLprotocol as HMTLprotocol
from hmtl.client import HMTLClient
from hmtl.TrianglePrograms import TriangleSnake,TriangleStatic

import argparse

def parse_options():
    parser = argparse.ArgumentParser()

    parser.add_argument('-v', '--verbose',
                        action="store_true", default=False,
                        help="Enable verbose output")

    group = parser.add_argument_group('Server options')
    group.add_argument("-p", "--port", type=int,
                        help="Port to bind to", default=6000)
    group.add_argument("-a", "--address",
                        default="localhost",
                        help="Address of server to connect to")

    # General options
    group = parser.add_argument_group('Target module')
    group.add_argument("-A", "--hmtladdress", type=int,
                      help="Address to which messages are sent [default=BROADCAST]",
                      default=HMTLprotocol.BROADCAST)
    group.add_argument("-O", "--output", action="store", type=int,
                       help="Number of the output to be set",
                       default=HMTLprotocol.OUTPUT_ALL_OUTPUTS)

    group = parser.add_argument_group('Commands')
    group.add_argument("-R", "--rgb", action="store_const",
                     dest="commandtype", const="rgb",
                     help="Set all to the indicated fg color")
    group.add_argument("-S", "--static", action="store_const",
                       dest="commandtype", const="static",
                       help="Static noise")
    group.add_argument("-T", "--snake", action="store_const",
                       dest="commandtype", const="snake",
                       help="Snake")
    group.add_argument("-N", "--none", action="store_const",
                     dest="commandtype", const="none",
                     help="Send program reset command")

    # Parameters for the commands
    group = parser.add_argument_group('Commands options')
    group.add_argument("-P", "--period", type=int,
                       default=100,
                       help="Set the period (in milliseconds)")
    group.add_argument("-f", "--foreground", action="store",
                       default="255,255,255",
                       help="Set the foreground color (r,g,b)")
    group.add_argument("-b", "--background", action="store",
                       default="0,0,0",
                       help="Set the background color (r,g,b)")
    group.add_argument("-t", "--threshold", type=int,
                       default=75,
                       help="Set a threshold value")
    group.add_argument("-c", "--colormode", type=int,
                       default=0,
                       help="Set a color mode")

    return parser.parse_args()

def main():
    options = parse_options()

    client = HMTLClient(address=options.address,
                        port=options.port,
                        hmtladdress=options.hmtladdress,
                        verbose=options.verbose)

    msg = None
    expect_response = False

    if options.commandtype == "rgb":
        if not options.foreground:
            print("RGB command requires foreground color")
            exit(1)
        fg = [ int(x) for x in options.foreground.split(",") ]
        print("Sending RGB message.  Address=%d Output=%d fg=%s" %
              (options.hmtladdress, options.output, fg))
        msg = HMTLprotocol.get_rgb_msg(options.hmtladdress,
                                       options.output,
                                       fg[0], fg[1], fg[2])

    if options.commandtype == "static":
        if not options.foreground or not options.background or not options.period or (options.threshold == None):
            print("STATIC command requires foreground, background, period, and threshold")
            exit(1)
        fg = [ int(x) for x in options.foreground.split(",") ]
        bg = [ int(x) for x in options.background.split(",") ]
        print("Sending STATIC message.  Address=%d Output=%d period=%d fg=%s bg=%s threshold=%d" %
              (options.hmtladdress, options.output, options.period, fg, bg,
               options.threshold))
        static = TriangleStatic(options.period, fg, bg,
                                options.threshold)
        msg = static.msg(options.hmtladdress, options.output)

    elif options.commandtype == "snake":
        if not options.background or not options.period or \
                (options.colormode == None):
            print("SNAKE command requires background, colormode, and period")
            exit(1)
        bg = [ int(x) for x in options.background.split(",") ]
        print("Sending STATIC message.  Address=%d Output=%d period=%d bg=%s colormode=%d" %
              (options.hmtladdress, options.output, options.period, bg,
               options.colormode))
        snake = TriangleSnake(options.period, bg, options.colormode)
        msg = snake.msg(options.hmtladdress, options.output)


    elif options.commandtype == "none":
        print("Sending NONE message.  Output=%d" % (options.output))
        msg = HMTLprotocol.get_program_none_msg(options.hmtladdress,
                                                options.output)

    if (msg != None):
        starttime = time.time()
        client.send_and_ack(msg, expect_response)
        endtime = time.time()
        print("Sent and acked in %.6fs" % (endtime - starttime))

    client.close()

    print("Done")
    exit(0)

if __name__ == '__main__':
    main()
