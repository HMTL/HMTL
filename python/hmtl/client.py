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

    def __init__(self, address='localhost', port=6000, hmtladdress=None, verbose=False, logger=True):
        self.logger = TimedLogger()
        if not logger:
            self.logger.disable()

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
        if self.verbose:
            self.logger.log(" - Waiting on ack")

        # Wait for an ack from the server
        has_ack = False
        while True:
            if self.get_ack():
                has_ack = True
                break

        if has_ack and expect_response:
            # If a response is expected then request the response data from the
            # server, repeating until the message indicates that there is no
            # more data.

            messages = []
            headers = []

            more_data = True
            while more_data:
                more_data = False

                # Request response data
                msg = self.get_response_data()
                messages.append(msg)

                if self.verbose:
                    if msg:
                            self.logger.log(" - Received data response: '%s':\n%s" %
                                  (hexlify(msg), HMTLprotocol.decode_data(msg)))
                    else:
                        self.logger.log(" - Failed to receive data response")

                try:
                    # Attempt to decode the message
                    decoded_msg = HMTLprotocol.msg_to_headers(msg)
                    if decoded_msg:
                        headers.append(decoded_msg)

                        # Check if the message indicate that there are
                        # additional messages expected.
                        hdr = decoded_msg[0]
                        if isinstance(hdr, HMTLprotocol.MsgHdr):
                            if hdr.more_data():
                                # Request another message
                                more_data = True
                    else:
                        headers.append(None)
                except Exception as e:
                    print("Exception decoding messages: %s" % (str(e)))
                    headers.append(None)

            return messages, headers

        return [None, None]

    def get_response_data(self):
        '''Request and attempt to retrieve response data'''
        self.conn.send(server.SERVER_DATA_REQ)
        #TODO: This should pickle some object with parameters like timeout
        msg = self.conn.recv()
        return msg
        

    def send_exit(self):
        self.send_and_ack(server.SERVER_EXIT)
