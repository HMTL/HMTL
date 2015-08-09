from __future__ import print_function

from binascii import hexlify
import serial
import threading
import time

from CircularBuffer import CircularBuffer
import HMTLprotocol


class SerialBuffer(threading.Thread):
    """
    This class reads from a serial port into a circular buffer.  It reads
    until it gets to the end of a line or the end of an HMTL message.
    """

    def __init__(self, device, baud=9600, timeout=0.1, bufflen=1000, verbose=True):
        threading.Thread.__init__(self)

        self.verbose = verbose

        self.last_received = 0
        self.total_received = 0

        # Create the buffer for storing serial data
        self.buff = CircularBuffer(bufflen)

        # Open the serial connection
        self.connection = serial.Serial(device, baud, timeout=timeout)
        print("SerialBuffer: connected to %s" % device)

        self.start_time = time.time()

        # Set as a daemon so that this thread will exit correctly
        # when the parent receives a kill signal
        self.daemon = True

    def get_buffer(self):
        return self.buff

    def get(self):
        return self.buff.get()

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
                    self.print_item(item)

    def print_item(self, item, color = "blue"):
        # Add a beginning of line timestamp
        self.print_color("[%.3f] " % (item.timestamp - self.start_time),
                         color="green", end="")

        if item.is_hmtl:
            self.print_color("%s : '%s' (raw)" %
                             (item.data, hexlify(item.data)),
                             color)
        else:
            try:
                self.print_color(item.data.decode(), color)
            except UnicodeDecodeError:
                self.print_color("%s : '%s' (raw)" %
                                 (item.data, hexlify(item.data)),
                                 color)

    def print_color(self, str, color = "blue", end="\n"):
        if color.lower() == "blue":
            print('\033[94m', end="")
        elif color.lower() == "red":
            print('\033[91m', end="")
        elif color.lower() == "green":
            print('\033[92m', end="")
        print(str, end="")
        print('\033[97m', end=end)


class SerialItem:
    """
    Class containing data from a single serial item
    """

    def __init__(self, data, timestamp, is_hmtl=False):
        self.data = data
        self.timestamp = timestamp
        self.is_hmtl = is_hmtl