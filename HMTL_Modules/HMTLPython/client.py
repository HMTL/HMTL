#
# Basic HMTL client
#

from multiprocessing.connection import Client
import time
import random
from binascii import hexlify

import HMTLprotocol
import server

class HMTLClient():
    setrgb = False
    period = 1.0

    def __init__(self, options):
        if (options.setrgb):
            self.setrgb = True
        if (options.period):
            self.period = options.period

        address = ('localhost', 6000)
        try:
            self.conn = Client(address, authkey='secret password')
        except Exception as e:
            raise Exception("Failed to connect to '%s'" % (str(address)))
        random.seed()

    def start(self):
        output = 0
        try:
            if (not self.setrgb):
                while True:
                    print("Turning output %d on" % (output))

                    command = HMTLprotocol.get_value_msg(output, 
                                                         HMTLprotocol.BROADCAST, 255)
                    print("  sending: %s" % (hexlify(command)))
                    self.send_and_ack(command)
                    time.sleep(self.period)

                    print("Turning output %d off" % (output))
                    command = HMTLprotocol.get_value_msg(output, 
                                                         HMTLprotocol.BROADCAST, 0)
                    print("  sending: %s" % (hexlify(command)))
                    self.send_and_ack(command)
                    output = (output + 1) % 4
            else:
                red = 255
                green = 0
                blue = 0
                while True:
                    print("Setting RGB output to %d,%d,%d" % (red, green, blue))
                    command = HMTLprotocol.get_rgb_msg(4, HMTLprotocol.BROADCAST,
                                                       red, green, blue)
                    self.send_and_ack(command)
                    time.sleep(self.period)

                    red = random.randrange(255)
                    green = random.randrange(255)
                    blue = random.randrange(255)

        except KeyboardInterrupt:
            print("Exiting")

        self.conn.send(server.SERVER_EXIT)

    def wait(self):
        self.conn.close()

    def send_and_ack(self, msg):
        self.conn.send(msg)

        # Wait for message acknowledgement
        while True:
            msg = self.conn.recv()
            print("Received: '%s' '%s'" % (msg, hexlify(msg)))
            if (msg == server.SERVER_ACK):
                break
