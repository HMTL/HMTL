#
# Basic HMTL client
#

from __future__ import print_function
#from __future__ import unicode_literals

from multiprocessing.connection import Client
import time
import random
from binascii import hexlify

import HMTLprotocol
import server

class HMTLClient():
    setrgb = False
    period = 1.0
    address = HMTLprotocol.BROADCAST
    verbose = False

    def __init__(self, options):
        if (options.setrgb):
            self.setrgb = True
        if (options.period):
            self.period = options.period
        if (options.hmtladdress != None):
            self.address = options.hmtladdress
        if (options.verbose):
            self.verbose = True
        if (options.port):
            port = options.port
        else:
            port = 6000

        address = ('localhost', port)
        try:
            self.conn = Client(address, authkey=b'secret password')
        except Exception as e:
            raise Exception("Failed to connect to '%s'" % (str(address)))
        random.seed()

    def test(self):
        output = 0
        try:
            if (not self.setrgb):
                while True:
                    print("Turning output %d on" % (output))

                    command = HMTLprotocol.get_value_msg(self.address, output, 
                                                         255)
                    self.send_and_ack(command)
                    time.sleep(self.period)

                    print("Turning output %d off" % (output))
                    command = HMTLprotocol.get_value_msg(self.address, output, 
                                                         0)
                    self.send_and_ack(command)
                    output = (output + 1) % 4
            else:
                red = 255
                green = 0
                blue = 0
                while True:
                    print("Setting RGB output to %d,%d,%d" % (red, green, blue))
                    command = HMTLprotocol.get_rgb_msg(self.address, 4,
                                                       red, green, blue)
                    self.send_and_ack(command)
                    time.sleep(self.period)

                    red = random.randrange(0,2)*255
                    green = random.randrange(0,2)*255
                    blue = random.randrange(0,2)*255

        except KeyboardInterrupt:
            print("Exiting")

    def close(self):
        self.conn.close()

    def send(self, msg):
        if (self.verbose):
            print(" - Sending %s" % (hexlify(msg)))

        self.conn.send(msg)

    def get_ack(self):
        msg = self.conn.recv()
        if (self.verbose):
            print(" - Received: '%s' '%s'" % (msg, hexlify(msg)))
        if (msg == server.SERVER_ACK):
            return True
        else:
            return False


    def send_and_ack(self, msg, expect_response=False):
        self.send(msg)

        # Wait for message acknowledgement
        if (self.verbose):
            print(" - Waiting on ack")
        while True:
            if self.get_ack():
                if (expect_response):
                    # Request response data
                    msg = self.get_response_data()
                    if (msg):
                        print(" - Received data response: '%s':\n%s" % 
                              (hexlify(msg), HMTLprotocol.decode_data(msg)))
                    else:
                        print(" - Failed to receive data response")
                    return msg
                else:
                    break

    def get_response_data(self):
        '''Request and attempt to retrieve response data'''
        self.conn.send(server.SERVER_DATA_REQ)
        msg = self.conn.recv()
        return msg
        

    def send_exit(self):
        self.send_and_ack(server.SERVER_EXIT)
