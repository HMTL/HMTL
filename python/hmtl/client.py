################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2014
#
# Basic HMTL module client library
#
################################################################################

from __future__ import print_function
#from __future__ import unicode_literals

from multiprocessing.connection import Client
import random
from binascii import hexlify

import HMTLprotocol
import server
from TimedLogger import TimedLogger

class HMTLClient():
    address = HMTLprotocol.BROADCAST
    verbose = False

    def __init__(self, address='localhost', port=6000, hmtladdress=None, verbose=False):
        self.logger = TimedLogger()

        self.address = hmtladdress
        self.verbose = verbose

        address = (address, port)
        try:
            self.conn = Client(address, authkey=b'secret password')
        except Exception as e:
            raise Exception("Failed to connect to '%s'" % (str(address)))
        random.seed()
        print("HMTLClient initialized")

    def close(self):
        self.conn.close()

    def send(self, msg):
        if (self.verbose):
            self.logger.log(" - Sending %s" % (hexlify(msg)))

        self.conn.send(msg)

    def get_ack(self):
        msg = self.conn.recv()
        if (self.verbose):
            self.logger.log(" - Received: '%s' '%s'" % (msg, hexlify(msg)))
        if (msg == server.SERVER_ACK):
            return True
        else:
            return False


    def send_and_ack(self, msg, expect_response=False):
        self.send(msg)

        # Wait for message acknowledgement
        if (self.verbose):
            self.logger.log(" - Waiting on ack")
        while True:
            if self.get_ack():
                if (expect_response):
                    # Request response data
                    msg = self.get_response_data()
                    if (msg):
                        self.logger.log(" - Received data response: '%s':\n%s" %
                              (hexlify(msg), HMTLprotocol.decode_data(msg)))
                    else:
                        self.logger.log(" - Failed to receive data response")
                    return msg
                else:
                    break

    def get_response_data(self):
        '''Request and attempt to retrieve response data'''
        self.conn.send(server.SERVER_DATA_REQ)
        #TODO: This should pickle some object with parameters like timeout
        msg = self.conn.recv()
        return msg
        

    def send_exit(self):
        self.send_and_ack(server.SERVER_EXIT)
