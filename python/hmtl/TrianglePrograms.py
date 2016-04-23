import hmtl.HMTLprotocol as HMTLprotocol
from abc import ABCMeta, abstractmethod
import struct

class TriangleProgram(HMTLprotocol.Msg):
    __metaclass__ = ABCMeta

    def __init__(self):
        pass

    @abstractmethod
    def pack(self):
        pass

    def get_code(self):
        return self.PROGRAM_CODE

    def msg(self, address, output):
        hdr = HMTLprotocol.MsgHdr(length=HMTLprotocol.MsgHdr.LENGTH + HMTLprotocol.ProgramHdr.LENGTH,
                                  mtype=HMTLprotocol.MSG_TYPE_OUTPUT,
                                  address=address)
        program_hdr = HMTLprotocol.ProgramHdr(self.get_code(), output)

        return hdr.pack() + program_hdr.pack() + self.pack()


class TriangleStatic(TriangleProgram):
    PROGRAM_CODE = 33
    TYPE = "TRIANGLE_STATIC"
    FORMAT = "<H" + "BBB" + "BBB" + "B" + "xxx"

    def __init__(self, period, foreground, background, threshold):
        self.period = period
        self.foreground = foreground
        self.background = background
        self.threshold = threshold

    def pack(self):
        return struct.pack(self.FORMAT,
                           self.period,
                           self.background[0],
                           self.background[1],
                           self.background[2],
                           self.foreground[0],
                           self.foreground[1],
                           self.foreground[2],
                           self.threshold)


class TriangleSnake(TriangleProgram):
    PROGRAM_CODE = 34
    TYPE = "TRIANGLE_SNAKE"
    FORMAT = "<H" + "BBB" + "B" + "xxxxxx"

    def __init__(self, period, background, colormode):
        self.period = period
        self.background = background
        self.colormode = colormode

    def pack(self):
        return struct.pack(self.FORMAT,
                           self.period,
                           self.background[0],
                           self.background[1],
                           self.background[2],
                           self.colormode)

