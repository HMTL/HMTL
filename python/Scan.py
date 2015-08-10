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

    # General options
    parser.add_option("-A", "--hmtladdress", dest="hmtladdress", type="int",
                      help="Address to which messages are sent [default=BROADCAST]", 
                      default=HMTLprotocol.BROADCAST)

    # Scan options
    parser.add_option("-e", "--scanevery", dest="scanevery", action="store_true",
                      help="Individual scan through address", default=False)


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

    if options.scanevery:
        modules = scan_every(client)
    else:
        modules = scan_broadcast(client)

    client.close()

    print("\nFound %d modules:" % (len(modules)))
    print("  %s" % (HMTLprotocol.PollHdr.headers()))
    for m in modules:
        print("  %s" % (m.dump()))

    exit(0)

main()
