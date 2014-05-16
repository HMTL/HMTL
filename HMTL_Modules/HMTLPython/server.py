import sys
sys.path.append("/Users/amp/Dropbox/Arduino/HMTL/HMTL_Modules/HMTLPython")

from multiprocessing.connection import Listener
import time
import serial
import HMTLprotocol

class HMTLServer():
    def __init__(self, options):
        address = (options.address, options.port)

#        address = ('localhost', 6000)     # family is deduced to be 'AF_INET'
        self.listener = Listener(address, authkey='secret password')
        self.conn = self.listener.accept()
        print 'connection accepted from', self.listener.last_accepted

    def start(self):
        print("Server started")
        while True:
            msg = self.conn.recv()
            print("Received: %s" % (msg))
            if msg == 'close':
                self.conn.close()
                break


    def wait(self):
        time.sleep(1)
        self.listener.close()
