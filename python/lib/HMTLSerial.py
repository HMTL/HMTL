#
# Class for handling the serial communications with an HMTL device
#

import serial
import time
import HMTLprotocol
from binascii import hexlify

class HMTLConfigException(Exception):
    pass

class HMTLSerial():
    ser = None

    def __init__(self, device, timeout=10, verbose=False, dryrun=False, baud = 9600):
        '''Open a serial connection and wait for the ready signal'''
        self.device = device
        self.verbose = verbose
        self.dryrun = dryrun
        self.baud = baud
        self.last_received = 0

        if (not self.dryrun):
            self.ser = serial.Serial(device, baud, timeout=timeout)
            if (self.wait_for_ready() == False):
                exit(1)
        else:
            print("HMTLSerial: initialized in dry-run mode")

    def vprint(self, str):
        if (self.verbose):
            print('\033[91m' + str + '\033[97m')

    def get_line(self, timeout=None):
        '''Returns the next line of text or a complete HMTL message'''

        if (timeout != None):
            prev_timeout = self.ser.ser.timeout
            self.ser.ser.timeout = 0.1

        data = ""
        is_msg_hdr = False
        hdr = None
        while True:
            char = self.ser.read(1)

            if (len(char) == 0):
                break

            if (ord(char) == HMTLprotocol.MsgHdr.STARTCODE):
                # This is the start of a data message
                is_msg_hdr = True
            if (is_msg_hdr):
                data += char

                if (len(data) == HMTLprotocol.MsgHdr.length()):
                    # Received enough data for a full message header
                    hdr = HMTLprotocol.MsgHdr.from_data(data)
                if (hdr != None):
                    if (len(data) >= hdr.length):
                        # Reached end of message
                        break
            else:
                # Arduino output lines are terminated with \r\n
                if (char == '\r'):
                    continue
                if (char == '\n'):
                    break
                data += char

        if (is_msg_hdr):
            self.vprint("  - received raw '%s' : '%s'" % (data, hexlify(data)))
            retdata = data
        else:
            try:
                retdata = data.decode()
                self.vprint("  - received '%s'" % (retdata))
            except UnicodeDecodeError:
                self.vprint("  - received raw '%s' : '%s'" % (data, hexlify(data)))
                retdata = data

        self.last_received = time.time()

        if (timeout != None):
            self.ser.ser.timeout = prev_timeout

        return retdata

    # Wait for data from device indicating its ready for commands
    def wait_for_ready(self):
        """Wait for the Arduino to send its ready signal"""
        print("***** Waiting for ready from Arduino *****")
        while True:
            data = self.get_line()
            if (len(data) == 0):
                raise Exception("Receive returned empty, timed out")
            if (data == HMTLprotocol.HMTL_CONFIG_READY):
                return True

    # Send terminated data and wait for (N)ACK
    def send_and_confirm(self, data, terminated):
        """Send a command and wait for the ACK"""

        if (self.dryrun):
            return True

        self.ser.write(data)
        if (terminated):
            self.ser.write(HMTLprotocol.HMTL_TERMINATOR)

        while True:
            ack = self.get_line()
            if (ack == HMTLprotocol.HMTL_CONFIG_ACK):
                return True
            if (ack == HMTLprotocol.HMTL_CONFIG_FAIL):
                raise HMTLConfigException("Configuration command failed")

# XXX: Here we need a method of getting data back from poll or the like

    # Send a text command
    def send_command(self, command):
        print("send_command: %s" % (command))
        #    data = bytes(command, 'utf-8')
        #    send_and_confirm(data)
        self.send_and_confirm(command, True)

    # Send a binary config update
    def send_config(self, type, config):
        print("send_config:  %-10s %s" % (type, hexlify(config)))
        self.send_and_confirm(config, True)

    # Flush the receive buffer
    def recv_flush(self):
        if (self.ser.inWaiting()):
            return self.get_line()
        return None
