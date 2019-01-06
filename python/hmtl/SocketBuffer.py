################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2017
#
# This class reads from a socket into a circular message buffer.  The
# data read can either be line terminated ('\n') or as HMTL messages
################################################################################

from __future__ import print_function

import socket

from TimedLogger import TimedLogger
from InputBuffer import InputBuffer


class SocketBuffer(InputBuffer):
    """
    This class reads from a socket into a circular buffer.  It reads
    until it gets to the end of a line or the end of an HMTL message.
    """

    # Default logging color
    LOGGING_COLOR = TimedLogger.RED

    def __init__(self, address, port,
                 timeout=0.1, bufflen=1000,
                 verbose=True):
        InputBuffer.__init__(self, bufflen, verbose)

        self.address = (address, port)

        # Open the serial connection
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # TODO: The timeout argument is from SerialBuffer, where the timeout is
        # for a particular serial.read() call.  This should do something similar
        self.sock.settimeout(30)
        self.sock.connect(self.address)

        self.logger.log("SocketBuffer: connected to %s:%d" % self.address,
                        color=TimedLogger.CYAN)

    def get_reader(self):
        return self.sock

    def read(self, max_read):
        return self.sock.recv(max_read)

    def write(self, data):
        return self.sock.sendall(data)

    def stop(self):
        self.sock.close()
        super(SocketBuffer, self).stop()
