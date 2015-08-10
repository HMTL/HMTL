from multiprocessing.connection import Listener
import threading

import HMTLprotocol
from HMTLSerial import *
from TimedLogger import TimedLogger

SERVER_ACK = "ack"
SERVER_EXIT = "exit"
SERVER_DATA_REQ = "data"


class HMTLServer():
    address = ('localhost', 6000)

    # Default logging color
    LOGGING_COLOR = TimedLogger.RED

    def __init__(self, options):
        # Connect to serial connection
        self.ser = HMTLSerial(options.device,
                              verbose=options.verbose,
                              baud=options.baud)
        self.logger = TimedLogger(self.ser.serial.start_time,
                                  textcolor=self.LOGGING_COLOR)

        self.address = (options.address, options.port)
        self.terminate = False

         # TODO: Is this needed at all? If so should the SerialBuffer handle synchronization?
        self.serial_cv = threading.Condition()

        self.conn = None
        self.listener = None

    def get_connection(self):
        try:
            self.logger.log("Waiting for connection")
            self.listener = Listener(self.address, authkey='secret password')
            self.conn = self.listener.accept()
            self.logger.log("Connection accepted from %s:%d" %
                            self.listener.last_accepted)
        except KeyboardInterrupt:
            print("Exiting")
            self.terminate = True

    def handle_msg(self, msg):
        if msg == SERVER_EXIT:
            self.logger.log("* Received exit signal *")
            self.conn.send(SERVER_ACK)
            self.close()
        elif msg == SERVER_DATA_REQ:
            self.logger.log("* Recieved data request")
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
        self.logger.log("Server started")
        self.get_connection()

        while not self.terminate:
            try:
                msg = self.conn.recv()
                self.logger.log("Received '%s'" % hexlify(msg))
                self.handle_msg(msg)
                self.logger.log("Acked '%s'" % hexlify(msg))

            except (EOFError, IOError):
                # Attempt to reconnect
                self.logger.log("Lost connection")
                self.listener.close()
                self.get_connection()
            except Exception as e:
                # Close the connection on uncaught exception
                self.logger.log("Exception during listen")
                self.close()
                raise e

    def close(self):
        self.listener.close()
        if self.conn:
            self.conn.close()
        self.terminate = True

    def get_data_msg(self):
        """Listen on the serial device for a properly formatted data message"""

        self.serial_cv.acquire()
        self.logger.log("Starting data request")
        
        time_limit = time.time() + 0.5
        msg = None
        while True:

            # Try to get a message with low timeout
            msg = self.ser.get_message(timeout=0.1)

            if msg and len(msg) > 0 and (ord(msg[0]) == HMTLprotocol.MsgHdr.STARTCODE):
                self.logger.log("Received response: '%s':\n%s" %
                                (hexlify(msg), HMTLprotocol.decode_data(msg)))
                break

            # Check for timeout
            if (time.time() > time_limit):
                self.logger.log("Data request time limit exceeded")
                msg = None
                break

        self.serial_cv.release()

        return msg