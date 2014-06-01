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
            print("[%.3f] Waiting for connection" % (time.time() - self.starttime))
            # XXX - We need to perform some form of background flush of the
            # XXX - serial connection?  May be behind lockups.
            self.listener = Listener(self.address, authkey='secret password')
            self.conn = self.listener.accept()
            print("[%.3f]  Connection accepted from %s" % 
                  ((time.time() - self.starttime), self.listener.last_accepted))
        except KeyboardInterrupt:
            print("Exiting")
            self.terminate = True

    def handle_msg(self, msg):
        if msg == SERVER_EXIT:
            print("* Received exit signal *")
            self.conn.send(SERVER_ACK)
            self.conn.close()
            self.terminate = True
        else:
            # Forward the message to the device
            self.ser.send_and_confirm(msg, False)

            # Reply with acknowledgement
            self.conn.send(SERVER_ACK)


    # Wait for and handle incoming connections
    def listen(self):
        self.starttime = time.time()

        print("[%.3f] Server started" % (time.time() - self.starttime))
        self.get_connection()

        while (not self.terminate):
            try:
                msg = self.conn.recv()
                print("[%.3f] Received '%s'" % (time.time() - self.starttime, hexlify(msg)))
                self.handle_msg(msg)
                print("[%.3f] Acked '%s'" % (time.time() - self.starttime, hexlify(msg)))

            except (EOFError, IOError):
                # Attempt to reconnect
                print("[%.3f] Lost connection" % (time.time() - self.starttime))
                self.listener.close()
                self.get_connection()

    def close(self):
        self.listener.close()
