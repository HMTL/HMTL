################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2014
#
# Class for handling the serial communications with an HMTL device
#
################################################################################

from binascii import hexlify
import time

import hmtl.HMTLprotocol as HMTLprotocol
from hmtl.TimedLogger import TimedLogger


class HMTLConfigException(Exception):
    pass


class HMTLSerial():

    # Default logging color
    LOGGING_COLOR = TimedLogger.WHITE

    # How long to wait for the ready signal after connection
    MAX_READY_WAIT = 10

    def __init__(self, buff, verbose=False):
        '''Open a serial connection and wait for the ready signal'''
        self.verbose = verbose
        self.last_received = 0
        self.serial = buff

        # Create the logger
        self.logger = TimedLogger(self.serial.start_time, textcolor=self.LOGGING_COLOR)

        self.serial.start()
        if not self.wait_for_ready():
            exit(1)

    def get_message(self, timeout=None):
        """Returns the next line of text or a complete HMTL message"""

        item = self.serial.get(wait=timeout)

        if not item:
            return None

        self.last_received = time.time()

        return item

    # Wait for data from device indicating its ready for commands
    def wait_for_ready(self):
        """Wait for the Arduino to send its ready signal"""
        self.logger.log("***** Waiting for ready from Arduino *****")
        start_wait = time.time()
        while True:
            item = self.get_message(1.0)

            # After connecting to the serial device there is sometimes data
            # from before the module resets, so wait a bit for that to clear.
            if (time.time() - start_wait) < 0.5:
                continue

            if item and (item.data == HMTLprotocol.HMTL_CONFIG_READY):
                self.logger.log("***** Recieved ready *****")
                return True
            if (time.time() - start_wait) > self.MAX_READY_WAIT:
                raise Exception("Timed out waiting for ready signal")

    # Send terminated data and wait for (N)ACK
    def send_and_confirm(self, data, terminated, timeout=10):
        """Send a command and wait for the ACK"""

        self.serial.write(data)
        if (terminated):
            self.serial.write(HMTLprotocol.HMTL_TERMINATOR)

        start_wait = time.time()
        while True:
            item = self.get_message()
            if item.data == HMTLprotocol.HMTL_CONFIG_ACK:
                return True
            if item.data == HMTLprotocol.HMTL_CONFIG_FAIL:
                raise HMTLConfigException("Configuration command failed")
            if (time.time() - start_wait) > timeout:
                raise Exception("Timed out waiting for ACK signal")


# XXX: Here we need a method of getting data back from poll or the like

    # Send a text command
    def send_command(self, command):
        self.logger.log("send_command: %s" % (command))
        self.send_and_confirm(command, True)

    # Send a binary config update
    def send_config(self, type, config):
        self.logger.log("send_config:  %-10s %s" % (type, hexlify(config)))
        self.send_and_confirm(config, True)