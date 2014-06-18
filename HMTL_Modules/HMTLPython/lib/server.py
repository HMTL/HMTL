import sys
sys.path.append("/Users/amp/Dropbox/Arduino/HMTL/HMTL_Modules/HMTLPython")

from multiprocessing.connection import Listener
import time
from binascii import hexlify
import serial
import threading

import HMTLprotocol
from HMTLSerial import *

SERVER_ACK = "ack"
SERVER_EXIT = "exit"

class HMTLServer():
    address = ('localhost', 6000)

    def __init__(self, options):
        # Connect to serial connection
        self.ser = HMTLSerial(options.device,
                              timeout=5,
                              verbose=options.verbose,
                              dryrun=options.dryrun)
        print("HMTLServer: connected to %s" % (options.device))

        self.address = (options.address, options.port)
        self.terminate = False
        self.serial_cv = threading.Condition()

        self.idle_thread = threading.Thread(target=self.serial_flush_idle, args=(0.1,'a'))
        self.idle_thread.start()

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
            self.serial_cv.acquire()
            self.ser.send_and_confirm(msg, False)
            self.serial_cv.release()

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

    # Read in all pending data on the serial device as a separate thread
    # XXX: Without this it seems to crash my Mac as the arduino continues to
    # stream data
    def serial_flush_idle(self, delay, test):
        print("Flush delay set to %f" % (delay))
        while (not self.terminate):
            # Read until the buffer appears to be empty
            while True:
                self.serial_cv.acquire()
                #            line = self.ser.get_line()
                readdata = self.ser.recv_flush()
                self.serial_cv.release()
                if (readdata):
                    print("[%.3f] received while idle" % (time.time() - self.starttime))
                else:
                    break

            # Delay before next check
            time.sleep(delay)
