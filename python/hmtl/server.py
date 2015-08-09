from multiprocessing.connection import Listener
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
                              baud=options.baud)

        self.address = (options.address, options.port)
        self.terminate = False
        self.serial_cv = threading.Condition()

        self.conn = None
        self.listener = None

    def elapsed(self):
        return time.time() - self.starttime

    def get_connection(self):
        try:
            print("[%.3f] Waiting for connection" % (self.elapsed()))
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
            print("* Recieved data request")
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
                print("[%.3f] Received '%s'" % (self.elapsed(), hexlify(msg)))
                self.handle_msg(msg)
                print("[%.3f] Acked '%s'" % (self.elapsed(), hexlify(msg)))

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
        if self.conn:
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