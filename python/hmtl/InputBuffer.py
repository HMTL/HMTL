################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2015
#
# This abstract class reads from a generic reader into a circular message
# buffer.  The data read can either be line terminated ('\n') or as HMTL
# messages.
#
################################################################################

from __future__ import print_function

from binascii import hexlify
import threading
import time

from CircularBuffer import CircularBuffer
from TimedLogger import TimedLogger
import HMTLprotocol

from abc import ABCMeta, abstractmethod


class InputBuffer(threading.Thread):
    __metaclass__ = ABCMeta

    # Default logging color
    LOGGING_COLOR = TimedLogger.CYAN

    def __init__(self, bufflen=1000, verbose=True):
        threading.Thread.__init__(self)

        self.verbose = verbose

        self.last_received = 0
        self.total_received = 0

        # Create the buffer for storing serial data
        self.buff = CircularBuffer(bufflen)

        self.start_time = time.time()
        self.logger = TimedLogger(self.start_time, textcolor=self.LOGGING_COLOR)

        # Set as a daemon so that this thread will exit correctly
        # when the parent receives a kill signal
        self.daemon = True

    @abstractmethod
    def get_reader(self):
        pass

    @abstractmethod
    def read(self, max_read):
        pass

    @abstractmethod
    def write(self, data):
        pass

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
                char = self.read(1)

                if (char is None) or (len(char) == 0):
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
                item = InputItem(data, self.last_received, is_html)
                self.buff.put(item)

                if self.verbose:
                    item.print(self.logger)


class InputItem:
    """
    Class containing data from a single serial item
    """

    def __init__(self, data, timestamp, is_hmtl=False):
        self.data = data
        self.timestamp = timestamp
        self.is_hmtl = is_hmtl
        self.hdr = HMTLprotocol.MsgHdr.from_data(data) if is_hmtl else None

    @staticmethod
    def from_data(data, timestamp=None):
        if not timestamp:
            timestamp = time.time()

        if len(data) == 0:
            return None

        if ord(data[0]) == HMTLprotocol.MsgHdr.STARTCODE:
            is_hmtl = True
            # TODO: Perform validation here
        else:
            is_hmtl = False

        return InputItem(data, timestamp, is_hmtl)

    def __str__(self):
        if self.is_hmtl:
            return "(%s) '%s'" % (self.hdr.msg_type(), hexlify(self.data))
        else:
            try:
                return self.data.decode()
            except UnicodeDecodeError:
                return "(raw) '%s'" % (hexlify(self.data))

    def print(self, logger, color=None):
        logger.log(str(self), self.timestamp, color)