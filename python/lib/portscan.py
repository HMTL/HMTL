

import os
import re
import serial
import fileinput
from serial.tools import list_ports

def serial_ports():
    """
    Returns a generator for all available serial ports
    """
    if os.name == 'nt':
        # windows
        for i in range(256):
            try:
                s = serial.Serial(i)
                s.close()
                yield 'COM' + str(i + 1)
            except serial.SerialException:
                pass
    else:
        # unix
        for port in list_ports.comports():
            yield port[0]

# List and choose a port from all available ports
def choose_port():
    ports = list(serial_ports())
    default = 1
    print("Available ports:")
    for (i, port) in enumerate(ports):
        print("  %d. %s" % (i + 1, port))
        if ((re.search("(.*usbserial.*)", port) != None) or
            (re.search("(.*usbmodem.*)", port) != None)):
            default = i + 1
            
    if len(ports) == 0:
        print("There are no ports to connect to")
        return None
    elif len(ports) == 1:
        useport = 1
    else:
        useport = -1

    while (useport < 0):
        try:
            val = raw_input("Select port [1..%d][%d]:" % (len(ports), default))
            if (len(val)):
                try:
                    num = int(val)
                    if (0 < num <= len(ports)):
                        useport = num
                except:
                    pass
            if (default and (len(val) == 0)):
                useport = default
        except:
            return None

    port = ports[useport - 1]
    print("Selected %d: %s" % (useport, port))
    return port

if __name__ == '__main__':
    print(list(serial_ports()))

