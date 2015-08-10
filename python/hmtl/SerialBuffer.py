from __future__ import print_function

from binascii import hexlify
import serial
import threading
import time

from CircularBuffer import CircularBuffer
from TimedLogger import TimedLogger
import HMTLprotocol


class SerialBuffer(threading.Thread):
    """
    This class reads from a serial port into a circular buffer.  It reads
    until it gets to the end of a line or the end of an HMTL message.
    """

    # Default logging color
    LOGGING_COLOR = TimedLogger.BLUE

    def __init__(self, device, baud=9600, timeout=0.1, bufflen=1000, verbose=True):
        threading.Thread.__init__(self)

        self.verbose = verbose

        self.last_received = 0
        self.total_received = 0

        # Create the buffer for storing serial data
        self.buff = CircularBuffer(bufflen)

        # Open the serial connection
        self.connection = serial.Serial(device, baud, timeout=timeout)

        self.start_time = time.time()
        self.logger = TimedLogger(self.start_time, textcolor=self.LOGGING_COLOR)

        self.logger.log("SerialBuffer: connected to %s" % device, color=TimedLogger.CYAN)

        # Set as a daemon so that this thread will exit correctly
        # when the parent receives a kill signal
        self.daemon = True

    def get_buffer(self):
        return self.buff

    def get(self, wait=None):
        return self.buff.get(wait=wait)

    def stop(self):
        self._Thread__stop()

    def run(self):
        while True:
            data = ""
            is_html = False
            hdr = None
            while True:
                char = self.connection.read(1)

                if len(char) == 0:
                    break

                self.total_received += 1

                if ord(char) == HMTLprotocol.MsgHdr.STARTCODE:
                    # This is the start of an HMTL data message
                    is_html = True
                if is_html:
                    data += char

                    if len(data) == HMTLprotocol.MsgHdr.length():
                        # Received enough data for a full message header
                        hdr = HMTLprotocol.MsgHdr.from_data(data)

                        # TODO: Perform basic header validation here
                    if hdr:
                        if len(data) >= hdr.length:
                            # Reached end of message
                            break
                else:
                    # Arduino print output lines are terminated with \r\n
                    if char == '\r':
                        continue
                    if char == '\n':
                        break
                    data += char

            if data and len(data):
                self.last_received = time.time()
                item = SerialItem(data, self.last_received, is_html)
                self.buff.put(item)

                if self.verbose:
                    item.print(self.logger)


class SerialItem:
    """
    Class containing data from a single serial item
    """

    def __init__(self, data, timestamp, is_hmtl=False):
        self.data = data
        self.timestamp = timestamp
        self.is_hmtl = is_hmtl

    def print(self, logger, color=None):
        if self.is_hmtl:
            logger.log("(raw) %s : '%s'" %
                       (self.data, hexlify(self.data)),
                       self.timestamp, color)
        else:
            try:
                logger.log(self.data.decode(), self.timestamp, color)
            except UnicodeDecodeError:
                logger.log("(raw) %s : '%s'" %
                           (self.data, hexlify(self.data)),
                           self.timestamp, color)