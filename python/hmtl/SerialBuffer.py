################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2015
#
# This class reads from a serial device into a circular message buffer.  The
# data read can either be line terminated ('\n') or as HMTL messages
################################################################################

import serial

from hmtl.TimedLogger import TimedLogger
from hmtl.InputBuffer import InputBuffer


class SerialBuffer(InputBuffer):
    """
    This class reads from a serial port into a circular buffer.  It reads
    until it gets to the end of a line or the end of an HMTL message.
    """

    # Default logging color
    LOGGING_COLOR = TimedLogger.CYAN

    def __init__(self, device, baud=9600, timeout=0.1, bufflen=1000,
                 verbose=True):
        InputBuffer.__init__(self, bufflen, verbose)

        # Open the serial connection
        self.connection = serial.Serial(device, baud, timeout=timeout)
        self.logger.log("SerialBuffer: connected to %s at %s baud" % (device, baud),
                        color=TimedLogger.CYAN)

    def get_reader(self):
        return self.connection

    def read(self, max_read):
        try:
            return self.connection.read(max_read)
        except serial.SerialException:
            return None

    def write(self, data):
        return self.connection.write(data)

    def stop(self):
        self.connection.close()
        super(SerialBuffer, self).stop()
