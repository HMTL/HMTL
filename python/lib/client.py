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
    address = HMTLprotocol.BROADCAST
    verbose = False

    def __init__(self, options):
        if (options.hmtladdress != None):
            self.address = options.hmtladdress
        if (options.verbose):
            self.verbose = True
        if (options.port):
            port = options.port
        else:
            port = 6000

        address = (options.address, port)
        try:
            self.conn = Client(address, authkey=b'secret password')
        except Exception as e:
            raise Exception("Failed to connect to '%s'" % (str(address)))
        random.seed()

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
