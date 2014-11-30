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
SERVER_DATA_REQ = "data"

class HMTLServer():
    address = ('localhost', 6000)

    def __init__(self, options):
        # Connect to serial connection
        self.ser = HMTLSerial(options.device,
                              timeout=options.timeout,
                              verbose=options.verbose,
                              dryrun=options.dryrun,
                              baud=options.baud)
        print("HMTLServer: connected to %s" % (options.device))

        self.address = (options.address, options.port)
        self.terminate = False
        self.serial_cv = threading.Condition()

        self.idle_thread = threading.Thread(target=self.serial_flush_idle, args=(0.1,'a'))
        self.idle_thread.start()

    def elapsed(self):
        return time.time() - self.starttime

    def get_connection(self):
        try:
            print("[%.3f] Waiting for connection" % (self.elapsed()))
            # XXX - We need to perform some form of background flush of the
            # XXX - serial connection?  May be behind lockups.
            self.listener = Listener(self.address, authkey='secret password')
            self.conn = self.listener.accept()
            print("[%.3f]  Connection accepted from %s" % 
                  ((self.elapsed()), self.listener.last_accepted))
        except KeyboardInterrupt:
            print("Exiting")
            self.terminate = True

    def handle_msg(self, msg):
        if msg == SERVER_EXIT:
            print("* Received exit signal *")
            self.conn.send(SERVER_ACK)
            self.close()
        elif (msg == SERVER_DATA_REQ):
            msg = self.get_data_msg()
            self.conn.send(msg)
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

        print("[%.3f] Server started" % (self.elapsed()))
        self.get_connection()

        while (not self.terminate):
            try:
                msg = self.conn.recv()
                print("[%.3f] Received '%s' '%s'" % (self.elapsed(), msg, hexlify(msg)))
                self.handle_msg(msg)
                print("[%.3f] Acked '%s' '%s'" % (self.elapsed(), msg, hexlify(msg)))

            except (EOFError, IOError):
                # Attempt to reconnect
                print("[%.3f] Lost connection" % (self.elapsed()))
                self.listener.close()
                self.get_connection()
            except Exception as e:
                # Close the connection on uncaught exception
                print("[%.3f] Exception during listen" % (self.elapsed()))
                self.close()
                raise e
                

    def close(self):
        self.listener.close()
        self.conn.close()
        self.terminate = True

    def get_data_msg(self):
        '''Listen on the serial device for a properly formatted data message'''

        self.serial_cv.acquire()
        print("[%.3f] Starting data request" % (self.elapsed()))
        
        self.time_limit = time.time() + 0.5;
        while True:

            # Try to get a message with low timeout
            msg = self.ser.get_line(timeout=0.1)

            if (len(msg) == 0):
                pass
            elif (msg[0] == chr(HMTLprotocol.MsgHdr.STARTCODE)):
                print("[%.3f] Received response: '%s':\n%s" % 
                      (self.elapsed(), hexlify(msg), 
                       HMTLprotocol.decode_data(msg)))
                break
            else:
                print("[%.3f] Received non-data message '%s':'%s'" % 
                      (self.elapsed(), HMTLprotocol.decode_data(msg), 
                       hexlify(msg)))

            # Check for timeout
            if (time.time() > self.time_limit):
                print("[%.3f] Data request time limit exceeded" % 
                      (self.elapsed()))
                msg = None
                break

        self.serial_cv.release()

        return msg


    # Read in all pending data on the serial device as a separate thread
    # XXX: Without this it seems to crash my Mac as the arduino continues to
    # stream data
    def serial_flush_idle(self, delay, test):
        print("Flush delay set to %f" % (delay))
        while (not self.terminate):
            # Read until the buffer appears to be empty
            while True:
                if (time.time() - self.ser.last_received < delay):
                    # Don't attempt to listen if data has come in within the
                    # delay period
                    break

                self.serial_cv.acquire()
                #            line = self.ser.get_line()
                readdata = self.ser.recv_flush()
                self.serial_cv.release()
                if (readdata):
                    print("[%.3f] received while idle: %s" % 
                          (self.elapsed(), hexlify(readdata)))
                else:
                    break

            # Delay before next check
            time.sleep(delay)
