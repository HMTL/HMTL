import sys
sys.path.append("/Users/amp/Dropbox/Arduino/HMTL/HMTL_Modules/HMTLPython")

from multiprocessing.connection import Listener
import time
from binascii import hexlify
import serial
import HMTLprotocol
from HMTLSerial import *

SERVER_ACK = "ack"
SERVER_EXIT = "exit"

class HMTLServer():
    def __init__(self, options):
        # Connect to serial connection
        self.ser = HMTLSerial(options.device, verbose=options.verbose,
                              dryrun=options.dryrun)
        print("HMTLSerial: connected to %s" % (options.device))

        address = (options.address, options.port)
        self.listener = Listener(address, authkey='secret password')
        self.conn = self.listener.accept()
        print("HMTLSerial: Connection accepted from",
              self.listener.last_accepted)

    def start(self):
        print("Server started")
        while True:
            msg = self.conn.recv()
            print("Received: '%s' '%s'" % (msg, hexlify(msg)))

            if msg == SERVER_EXIT:
                print("* Received exit signal *")
                self.conn.close()
                break
            else:
                # Forward the message to the device
                self.ser.send_and_confirm(msg, False)

                # Reply with acknowledgement
                self.conn.send(SERVER_ACK)


    def wait(self):
        self.listener.close()
