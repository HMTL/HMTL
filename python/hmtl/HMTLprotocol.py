################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2014
#
# Definitions of protocol for talking to HMTL modules via serial
#
################################################################################

"""HMTL Protocol definitions module"""

import struct
from binascii import hexlify
from constants import *
from config import *

# Protocol commands
HMTL_CONFIG_READY  = "ready"
HMTL_CONFIG_ACK    = "ok"
HMTL_CONFIG_FAIL   = "fail"

HMTL_TERMINATOR    = b'\xfe\xfe\xfe\xfe' # Indicates end of command


#
# HMTL Message formats
#
MSG_HDR_FMT = "<BBBBBBH" # All HMTL messages start with this

# Msg type
MSG_TYPE_OUTPUT   = 1
MSG_TYPE_POLL     = 2
MSG_TYPE_SET_ADDR = 3
MSG_TYPE_DUMPCONFIG = 0xE0

# Mapping of message types to strings
MSG_TYPES = {
    MSG_TYPE_OUTPUT: "OUTPUT",
    MSG_TYPE_POLL: "POLL",
    MSG_TYPE_SET_ADDR: "SETADDR",
    MSG_TYPE_DUMPCONFIG: "DUMPCONFIG",
}

# Msg flags
MSG_FLAG_ACK       = (1 << 0)
MSG_FLAG_RESPONSE  = (1 << 1)
MSG_FLAG_MORE_DATA = (1 << 2)
MSG_FLAG_ERROR     = (1 << 3)

# Mapping of message flags to strings
MSG_FLAGS = {
    MSG_FLAG_ACK: "ACK",
    MSG_FLAG_RESPONSE: "RESPONSE",
    MSG_FLAG_MORE_DATA: "MORE_DATA",
    MSG_FLAG_ERROR: "ERROR",
}

MSG_VALUE_FMT = "H"
MSG_RGB_FMT = "BBB"
MSG_PROGRAM_FMT = "B"

MSG_BASE_LEN = 8
MSG_OUTPUT_LEN = MSG_BASE_LEN + 2
MSG_VALUE_LEN = MSG_OUTPUT_LEN + 2
MSG_RGB_LEN = MSG_OUTPUT_LEN + 3

MSG_PROGRAM_VALUE_LEN = 12
MSG_PROGRAM_LEN = MSG_OUTPUT_LEN + 1 + MSG_PROGRAM_VALUE_LEN

MSG_POLL_LEN = MSG_BASE_LEN
MSG_DUMPCONFIG_LEN = MSG_BASE_LEN

# Broadcast address
BROADCAST = 65535  # = (uint16_t)-1

# Default Port for direct TCP connections
HMTL_PORT = 4365

# Specific program formats
MSG_PROGRAM_NONE_TYPE = 0
MSG_PROGRAM_NONE_FMT = 'B'*MSG_PROGRAM_VALUE_LEN

MSG_PROGRAM_BLINK_TYPE = 1
MSG_PROGRAM_BLINK_FMT = '<HBBBHBBB' + 'BB' # Msg + padding

MSG_PROGRAM_TIMED_CHANGE_TYPE = 2
MSG_PROGRAM_TIMED_CHANGE_FMT = '<LBBBBBB' + 'BB' # Msg + padding

MSG_PROGRAM_LEVEL_VALUE_TYPE = 3
MSG_PROGRAM_LEVEL_VALUE_FMT = '<BBBBBBBBBBBB' # Only padding

MSG_PROGRAM_SOUND_VALUE_TYPE = 4
MSG_PROGRAM_SOUND_VALUE_FMT = '<BBBBBBBBBBBB' # Only padding

MODULE_TYPES = {
    1 : "HMTL_Module",
    2 : "WirelessPendant",
    3 : "TriangleLightModule",
    4 : "HMTL_Fire_Control",
    128: "Lightbringer 328",
    129: "Lightbringer 1284"
}


#
# HMTL Message types
#

def get_msg_hdr(msglen, address, mtype=MSG_TYPE_OUTPUT, flags=0):
    packed = struct.pack(MSG_HDR_FMT,
                         0xFC,   # Startcode
                         0,      # CRC - XXX: TODO!
                         2,      # Protocol version
                         msglen, # Message length
                         mtype,  # Type: 1 is OUTPUT, 2 POLL, 3 is SETADDR
                         flags,  # flags
                         address)  # Destination address 65535 is "Any"
    return packed

def get_output_hdr(otype, output):
    packed = struct.pack(OUTPUT_HDR_FMT,
                         CONFIG_TYPES[otype], # Message type 
                         output)             # Output number
    return packed

def get_value_msg(address, output, value):
    packed_hdr = get_msg_hdr(MSG_VALUE_LEN, address)
    packed_out = get_output_hdr("value", output)
    packed = struct.pack(MSG_VALUE_FMT,
                         value)                 # Value to set

    return packed_hdr + packed_out + packed                        

def get_rgb_msg(address, output, r, g, b):
    packed_hdr = get_msg_hdr(MSG_RGB_LEN, address)
    packed_out = get_output_hdr("rgb", output)
    packed = struct.pack(MSG_RGB_FMT, r, g, b)

    return packed_hdr + packed_out + packed


def get_poll_msg(address):
    packed_hdr = get_msg_hdr(MSG_POLL_LEN, address,
                             mtype=MSG_TYPE_POLL,
                             flags=MSG_FLAG_RESPONSE)

    return packed_hdr


def get_dumpconfig_msg(address):
    packed_hdr = get_msg_hdr(MSG_DUMPCONFIG_LEN, address,
                             mtype=MSG_TYPE_DUMPCONFIG,
                             flags=MSG_FLAG_RESPONSE)

    return packed_hdr


def get_set_addr_msg(address, device_id, new_address):
    hdr = MsgHdr(length = MsgHdr.LENGTH + SetAddress.LENGTH,
                 mtype = MSG_TYPE_SET_ADDR, 
                 address = address)
    sethdr = SetAddress(device_id, new_address)

    return hdr.pack() + sethdr.pack()


def get_program_msg(address, output, program_type, program_data):
    if (len(program_data) != MSG_PROGRAM_VALUE_LEN):
        raise Exception("Program data must be %d bytes" % (MSG_PROGRAM_VALUE_LEN))

    packed_hdr = get_msg_hdr(MSG_PROGRAM_LEN, address)
    packed_out = get_output_hdr("program", output)
    packed = struct.pack(MSG_PROGRAM_FMT, program_type)

    return packed_hdr + packed_out + packed + program_data

def get_program_blink_msg(address, output, 
                          on_period, on_values, off_period, off_values):
    blink = struct.pack(MSG_PROGRAM_BLINK_FMT,
                        on_period, on_values[0], on_values[1], on_values[2],
                        off_period, off_values[0], off_values[1], off_values[2],
                        0, 0)
    return get_program_msg(address, output, MSG_PROGRAM_BLINK_TYPE, blink)
    
def get_program_none_msg(address, output):
    args = [MSG_PROGRAM_NONE_FMT] + [0 for x in range(0, MSG_PROGRAM_VALUE_LEN)]
    msg = struct.pack(*args)
    return get_program_msg(address, output, MSG_PROGRAM_NONE_TYPE, msg)

def get_program_timed_change_msg(address, output,
                                 change_period, start_values, stop_values):
    msg = struct.pack(MSG_PROGRAM_TIMED_CHANGE_FMT,
                      change_period,
                      start_values[0], start_values[1], start_values[2],
                      stop_values[0], stop_values[1],stop_values[2],
                      0, 0)
    return get_program_msg(address, output, MSG_PROGRAM_TIMED_CHANGE_TYPE, msg)


# Decode raw data into an HMTL message
def decode_data(readdata):
    try:
        text = readdata.decode()
    except UnicodeDecodeError:
        # Check to see if this is a valid HMTL message
        (text, msg) = decode_msg(readdata)

    return text


def msg_to_headers(data):
    """Attempt to get HMTL headers from data"""
    headers = []
    hdr = MsgHdr.from_data(data)
    headers.append(hdr)

    hdr = hdr.next_hdr(data)
    if (hdr != None):
        headers.append(hdr)

    return headers


def decode_msg(data):
    """Attempt to convert raw data to text and a HMTL header"""
    headers = msg_to_headers(data)

    text = ""
    for hdr in headers:
        text += str(hdr)

    return text, headers[-1]


#
# Serialization objects
#

# Abstract class for all message types
class Msg(object):
    @classmethod
    def from_data(cls, data, offset=0):
        header = struct.unpack_from(cls.FORMAT, data, offset)
        return cls(*header)

    @classmethod
    def length(cls):
        return cls.LENGTH

    @classmethod
    def type(cls):
        return cls.TYPE

#
# Header for messages sent to a TCPSocket object
#
class TCPSocketHeader(Msg):
    FORMAT = "<IBBBBHH"
    LEN = 12

    STARTCODE = 0x54435053
    VERSION = 1

    def __init__(self, id, datalen, source, dest, flags):
        self.id = id
        self.datalen = datalen
        self.source = source
        self.dest = dest
        self.flags = flags
        # 0x54435053,  # START
        # 1,  # VERSION
        # id,  # ID
        # 4,  # Data Length
        # 0x12,  # Source addr
        # 128,  # Dest addr
        # 0x56,  # Flags

    def __str__(self):
        return """  tcphdr:
  start:%04x
  version:%d
  id:%d
  datalen:%d
  source:%d
  dest:%d
  flags:0x%x
""" % (self.STARTCODE, self.VERSION, self.id,
       self.datalen, self.source, self.dest, self.flags)

    def pack(self):
        return struct.pack(self.FORMAT, self.STARTCODE, self.VERSION, self.id,
                           self.datalen, self.flags, self.source, self.dest)


# HMTL Message header
class MsgHdr(Msg):
    TYPE = "MSGHDR"
    FORMAT = MSG_HDR_FMT
    LENGTH =  MSG_BASE_LEN
    
    STARTCODE = 0xFC
    PROTOCOL_VERSION = 2

    def __init__(self, startcode=STARTCODE, crc=0, version=PROTOCOL_VERSION, 
                 length=0, mtype=0, flags=0, address=0):
        self.startcode = startcode
        self.crc = crc
        self.version = version
        self.length = length
        self.mtype = mtype
        self.flags = flags
        self.address = address

    def __str__(self):
        return """  msg_hdr_t:
    start:%02x
    crc:%02x
    version:%d
    len:%d
    type:%d (%s)
    flags:0x%x (%s)
    addr:%d
""" % (
            self.startcode,
            self.crc,
            self.version,
            self.length,
            self.mtype, MSG_TYPES[self.mtype],
            self.flags, '|'.join([MSG_FLAGS[1 << x] for x in range(0,8) if ((1 << x) & self.flags) ]),
            self.address
        )

    def pack(self):
        return struct.pack(self.FORMAT, self.startcode, self.crc, self.version, 
                           self.length, self.mtype, self.flags, self.address)

    def next_hdr(self, data):
        '''Return the header following the message header'''
        if (self.mtype == MSG_TYPE_OUTPUT):
            raise Exception("MSG_TYPE_OUTPUT currently not handled in parsing")
        elif (self.mtype == MSG_TYPE_POLL):
            return PollHdr.from_data(data, self.LENGTH)
        elif (self.mtype == MSG_TYPE_DUMPCONFIG):
            return DumpConfigHdr.from_data(data[self.LENGTH:])
        else:
            raise Exception("Unknown message type %d" % (self.mtype))

    def msg_type(self):
        """Return the string of the header's message type"""
        if self.mtype not in MSG_TYPES:
            print("ERROR: No message type %d" % self.mtype)
            return None
        return MSG_TYPES[self.mtype]

    def more_data(self):
        """Return whether the header indicates that this is part of a multi-packet
           message."""
        return self.flags & MSG_FLAG_MORE_DATA
        

class PollHdr(Msg):
    TYPE = "POLL"
    FORMAT = HEADER_FMT + "HHB"  # config_hdr_t + remainder of poll message
    LENGTH = 13

    def __init__(self, magic, protocol_version, hardware_version, baud, 
                 num_outputs, flags, device_id, address, 
                 object_type, buffer_size, msg_version):
        self.magic = magic;
        self.protocol_version = protocol_version
        self.hardware_version = hardware_version
        self.baud = baud
        self.num_outputs = num_outputs
        self.flags = flags
        self.device_id = device_id
        self.address = address

        self.object_type = object_type
        self.buffer_size = buffer_size
        self.msg_version = msg_version

    def __str__(self):
        return """  poll_hdr_t:
    config_hdr_t:
    magic:%02x
    proto_ver:%d
    hdw_ver:%d
    baud:%d
    outputs:%d
    flags:%02x
    dev_id:%d
    addr:%d
    type:%d
    buffer_size:%d
    msg_version=%d
""" %(self.magic,
      self.protocol_version,
      self.hardware_version,
      byte_to_baud(self.baud),
      self.num_outputs,
      self.flags,
      self.device_id,
      self.address,
      self.object_type,
      self.buffer_size,
      self.msg_version)

    @classmethod
    def headers(cls):
        return "%-8s %-8s %-8s %-8s %-8s %-8s %-8s" % ("device", "address", "protocol", "hardware", "baud", "outputs", "type")

    def dump(self):
        if self.object_type in MODULE_TYPES:
            module_type = MODULE_TYPES[self.object_type]
        else:
            module_type = "unknown(%s)" % self.object_type
        return "%-8d %-8d %-8d %-8d %-8d %-8d %-8s" % \
               (self.device_id, self.address, self.protocol_version,
                self.hardware_version, byte_to_baud(self.baud),
                self.num_outputs, module_type)


class SetAddress(Msg):
    TYPE = "SETADDR"
    FORMAT = "<HH"
    LENGTH = 4

    def __init__(self, device_id, address):
        self.device_id = device_id
        self.address = address

    def __str__(self):
        return ("  msg_set_addr_t:\n    dev_id:%d\n    addr:%d\n" % 
                (self.device_id, self.address))

    def pack(self):
        return struct.pack(self.FORMAT, self.device_id, self.address)


class DumpConfigHdr(Msg):
    """Message type for receiving configuration data from a module"""
    TYPE = "DUMPCONFIG"
    LENGTH = -1

    def __init__(self, data, config):
        self.data = data
        self.config = config

    @classmethod
    def from_data(cls, data, offset=0):
        if ord(data[0]) == HEADER_MAGIC:
            config = ConfigHeaderMain.from_data(data)
        else:
            config = cls.full_config(data)
        return cls(data, config)

    def __str__(self):
        return str(self.config)

    def short(self):
        return self.config.short()

    def type(self):
        return self.config.type()

    @staticmethod
    def full_config(data):
        output_hdr = OutputHdr.from_data(data)
        remaining_data = data[output_hdr.LENGTH:]

        config = None
        if output_hdr.outputtype == CONFIG_TYPES["value"]:
            config = ConfigHeaderValue.from_data(remaining_data)
        elif output_hdr.outputtype == CONFIG_TYPES["rgb"]:
            config = ConfigHeaderRGB.from_data(remaining_data)
        elif output_hdr.outputtype == CONFIG_TYPES["pixels"]:
            config = ConfigHeaderPixels.from_data(remaining_data)
        elif output_hdr.outputtype == CONFIG_TYPES["rs485"]:
            config = ConfigHeaderRS485.from_data(remaining_data)
        elif output_hdr.outputtype == CONFIG_TYPES["xbee"]:
            config = ConfigHeaderXbee.from_data(remaining_data)
        elif output_hdr.outputtype == CONFIG_TYPES["mpr121"]:
            config = ConfigHeaderMPR121.from_data(remaining_data)

        if config:
            config.output_hdr = output_hdr

        return config


class OutputHdr(Msg):
    TYPE = "OUTPUT"
    FORMAT = OUTPUT_HDR_FMT
    LENGTH = 2

    def __init__(self, outputtype, output):
        self.outputtype = outputtype
        self.output = output

    def __str__(self):
        return ("  output_hdr_t:\n    type:%d\n    output:%d\n" %
                (self.outputtype, self.output))

    def pack(self):
        return struct.pack(self.FORMAT, self.outputtype, self.output)


#
# Program message classes
#

class ProgramHdr(Msg):
    TYPE = "PROGRAM"
    FORMAT = "<B"
    MAX_DATA = 32 # Was 12 # XXX!
    LENGTH = OutputHdr.LENGTH + 1 + MAX_DATA

    def __init__(self, program, output):
        self.outputHdr = OutputHdr(CONFIG_TYPES["program"], output)
        self.program = program

    def __str__(self):
        return str(self.outputHdr) + ("  msg_program_t:\n    type:%d\n" % 
                                      (self.program))

    def pack(self):
        return self.outputHdr.pack() + struct.pack(self.FORMAT, self.program)

    @classmethod
    def from_data(cls, data, offset=0):
        raise Exception("From data needs to be defined for ProgramHdr")


class ProgramGeneric(Msg):
    """Generic program data message"""
    TYPE = "PROGRAMGENERIC"
    FORMAT = "<%s" % ('B' * ProgramHdr.MAX_DATA)

    NAME_MAP = {
        "blink":       0x01,
        "timed":       0x02,
        "level":       0x03,
        "sound":       0x04,
        "fade":        0x05,
        "sparkle":     0x06,
        "soundpixels": 0x07,
        "circular":    0x08,

        "brightness":  0x30,
        "color":       0x31,
    }

    def __init__(self, values=None):
        if values and (len(values) > ProgramHdr.MAX_DATA):
            raise Exception("Received more values (%d) than max (%d)" %
                            (len(values), ProgramHdr.MAX_DATA))

        if values:
            # Convert all values to ints and fill unspecified bytes as zero
            self.values = [int(values[i]) if i < len(values) else 0 for i in range(ProgramHdr.MAX_DATA)]
        else:
            self.values = [0 for _ in range(ProgramHdr.MAX_DATA)]
        pass

    def pack(self):
        return struct.pack(self.FORMAT, *self.values)


class ProgramLevelValue(ProgramGeneric):
    TYPE = "PROGRAMLEVELVALUE"


class ProgramSoundValue(ProgramGeneric):
    TYPE = "PROGRAMSOUNDVALUE"


class ProgramFade(Msg):
    TYPE = "PROGRAMFADE"
    TYPE_NUM = ProgramGeneric.NAME_MAP["fade"]

    BASE_FORMAT = 'LBBBBBBB'
    BASE_FORMAT_LENGTH = 11
    PADDING = ProgramHdr.MAX_DATA - BASE_FORMAT_LENGTH
    FORMAT = "<%s%s" % (BASE_FORMAT, 'B' * PADDING)

    def __init__(self, change_period, start_values, stop_values, flags):
        self.change_period = change_period
        self.start_values = start_values
        self.stop_values = stop_values
        self.flags = flags

    def pack(self):
        return struct.pack(self.FORMAT, self.change_period,
                           self.start_values[0], 
                           self.start_values[1], 
                           self.start_values[2],
                           self.stop_values[0],
                           self.stop_values[1],
                           self.stop_values[2],
                           self.flags,
                           *[0 for i in range(self.PADDING)])


class ProgramSparkle(Msg):
    TYPE = "PROGRAMSPARKLE"
    TYPE_NUM = ProgramGeneric.NAME_MAP["sparkle"]

    BASE_FORMAT = 'HBBBBBBBBBBB'
    BASE_FORMAT_LENGTH=13
    PADDING = ProgramHdr.MAX_DATA - BASE_FORMAT_LENGTH
    FORMAT = "<%s%s" % (BASE_FORMAT, 'B' * PADDING)

    def __init__(self, period, bg_values, sparkle_threshold, bg_threshold,
                 hue_min, hue_max, sat_min, sat_max, val_min, val_max):
        self.period = period
        self.bg_values = bg_values
        self.sparkle_threshold = sparkle_threshold
        self.bg_thresholds = bg_threshold
        self.hue_max = hue_max
        self.hue_min = hue_min
        self.sat_min = sat_min
        self.sat_max = sat_max
        self.val_min = val_min
        self.val_max = val_max

    def pack(self):
        print("FORMAT=%s PADDING=%d" % (self.FORMAT, self.PADDING))
        return struct.pack(self.FORMAT,
                           self.period,
                           self.bg_values[0],
                           self.bg_values[1],
                           self.bg_values[2],
                           self.sparkle_threshold,
                           self.bg_thresholds,
                           self.hue_min,
                           self.hue_max,
                           self.sat_min,
                           self.sat_max,
                           self.val_min,
                           self.val_max,
                           *[0 for i in range(self.PADDING)])

    def prepare_msg(self, address, output):
        hdr = MsgHdr(length=MsgHdr.LENGTH + ProgramHdr.LENGTH,
                     mtype=MSG_TYPE_OUTPUT,
                     address=address)
        programhdr = ProgramHdr(self.TYPE_NUM, output)
        return hdr.pack() + programhdr.pack() + self.pack()


class ProgramCircular(Msg):
    TYPE = "PROGRAMCIRCULAR"
    TYPE_NUM = ProgramGeneric.NAME_MAP["circular"]

    BASE_FORMAT = 'HHBBBBB'
    BASE_FORMAT_LENGTH = 9
    PADDING = ProgramHdr.MAX_DATA - BASE_FORMAT_LENGTH
    FORMAT = "<%s%s" % (BASE_FORMAT, 'B' * PADDING)

    def __init__(self, period, chain_length, bg_values, pattern, flags):
        self.period = period
        self.chain_length = chain_length
        self.bg_values = bg_values
        self.pattern = pattern
        self.flags = flags

    def pack(self):
        return struct.pack(self.FORMAT, self.period,
                           self.chain_length,
                           self.bg_values[0],
                           self.bg_values[1],
                           self.bg_values[2],
                           self.pattern,
                           self.flags,
                           *[0 for i in range(self.PADDING)])

    def prepare_msg(self, address, output):
        hdr = MsgHdr(length=MsgHdr.LENGTH + ProgramHdr.LENGTH,
                     mtype=MSG_TYPE_OUTPUT,
                     address=address)
        programhdr = ProgramHdr(self.TYPE_NUM, output)
        return hdr.pack() + programhdr.pack() + self.pack()


def get_program_level_value_msg(address, output):
    hdr = MsgHdr(length = MsgHdr.LENGTH + ProgramHdr.LENGTH,
                 mtype = MSG_TYPE_OUTPUT,
                 address = address)
    programhdr = ProgramHdr(MSG_PROGRAM_LEVEL_VALUE_TYPE, output)
    levelhdr = ProgramLevelValue()

    return hdr.pack() + programhdr.pack() + levelhdr.pack()


def get_program_sound_value_msg(address, output):
    hdr = MsgHdr(length = MsgHdr.LENGTH + ProgramHdr.LENGTH,
                 mtype = MSG_TYPE_OUTPUT,
                 address = address)
    programhdr = ProgramHdr(MSG_PROGRAM_SOUND_VALUE_TYPE, output)
    soundhdr = ProgramSoundValue()

    return hdr.pack() + programhdr.pack() + soundhdr.pack()


def get_program_fade_msg(address, output,
                         change_period, start_values, stop_values, flags=0):
    hdr = MsgHdr(length=MsgHdr.LENGTH + ProgramHdr.LENGTH,
                 mtype=MSG_TYPE_OUTPUT,
                 address=address)
    programhdr = ProgramHdr(ProgramFade.TYPE_NUM, output)
    fadehdr = ProgramFade(change_period, start_values, stop_values, flags)

    return hdr.pack() + programhdr.pack() + fadehdr.pack()


def get_program_generic(address, output, program, data):
    hdr = MsgHdr(length = MsgHdr.LENGTH + ProgramHdr.LENGTH,
                 mtype = MSG_TYPE_OUTPUT,
                 address = address)
    programhdr = ProgramHdr(program, output)
    datahdr = ProgramGeneric(data)

    return hdr.pack() + programhdr.pack() + datahdr.pack()
