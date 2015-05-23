#!/usr/bin/python
#
# Scan for all network addresses

import hmtl.HMTLprotocol as HMTLprotocol
from hmtl.client import HMTLClient

from optparse import OptionParser, OptionGroup

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)

    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))
    
    options.setrgb=False
    options.period=False
    options.s=False

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

    # Test mode
    parser.add_option("-t", "--testmode", dest="testmode", action="store_true",
                      help="Run test mode", default=False)
    parser.add_option("-r", "--rgbtest", dest="setrgb", action="store_true",
                      help="Set RGB value", default=False)

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

    return (options, args)

def handle_poll_resp(data, modules):
    (text, msg) = HMTLprotocol.decode_msg(data)
    if (isinstance(msg, HMTLprotocol.PollHdr)):
        print("Poll response: dev=%d addr=%d" % (msg.device_id, msg.address))
        modules.append(msg)
    else:
        print("Not a poll response, class: %s " % (msg.__class__.__name__))

def scan_every(client):
    modules = []
    for address in range(0,128):
        print("Polling address: %d" % (address))
        msg = HMTLprotocol.get_poll_msg(address)
        ret = client.send_and_ack(msg, True)
        if (ret):
            handle_poll_resp(ret, modules)
        else:
            print("No response from: %d" % (address))

    return modules

def scan_broadcast(client):
    modules = []
    msg = HMTLprotocol.get_poll_msg(HMTLprotocol.BROADCAST)
    
    ret = client.send_and_ack(msg, False)
    while (True):
        msg = client.get_response_data()
        if (msg == None):
            break

        handle_poll_resp(msg, modules)

    return modules
    

def main():
    (options, args) = handle_args()

    client = HMTLClient(options)

    #modules = scan_every(client)
    modules = scan_broadcast(client)

    client.close()

    print("\nFound %d modules:" % (len(modules)))
    print("  %s" % (HMTLprotocol.PollHdr.headers()))
    for m in modules:
        print("  %s" % (m.dump()))

    exit(0)

main()
