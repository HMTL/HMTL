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
    address = ('localhost', 6000)

    def __init__(self, options):
        # Connect to serial connection
        self.ser = HMTLSerial(options.device, verbose=options.verbose,
                              dryrun=options.dryrun)
        print("HMTLServer: connected to %s" % (options.device))

        self.address = (options.address, options.port)
        self.terminate = False

    def get_connection(self):
        try:
            print("Waiting for connection")
            self.listener = Listener(self.address, authkey='secret password')
            self.conn = self.listener.accept()
            print("* Connection accepted from", self.listener.last_accepted)
        except KeyboardInterrupt:
            print("Exiting")
            self.terminate = True

    def handle_msg(self, msg):
        if msg == SERVER_EXIT:
            print("* Received exit signal *")
            self.conn.close()
            self.terminate = True
        else:
            # Forward the message to the device
            self.ser.send_and_confirm(msg, False)

            # Reply with acknowledgement
            self.conn.send(SERVER_ACK)


    def start(self):
        print("Server started")
        self.get_connection()

        self.starttime = time.time()

        while (not self.terminate):
            try:
                msg = self.conn.recv()
                print("[%.2f] Received '%s'" % (time.time() - self.starttime, hexlify(msg)))
                self.handle_msg(msg)
                print("[%.2f] Acked '%s'" % (time.time() - self.starttime, hexlify(msg)))

            except (EOFError, IOError):
                # Attempt to reconnect
                print("Lost connection")
                self.listener.close()
                self.get_connection()

    def wait(self):
        self.listener.close()
