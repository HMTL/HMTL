from multiprocessing.connection import Client
import time

class HMTLClient():
    def __init__(self, options):
        address = ('localhost', 6000)
        try:
            self.conn = Client(address, authkey='secret password')
        except Exception as e:
            raise Exception("Failed to connect to '%s'" % (str(address)))
            

    def start(self):
        print("Client started")
        for i in range(0, 10):
            print("Sending %d" % (i))
            self.conn.send(i)
            time.sleep(1)
        self.conn.send("close")

    def wait(self):
        time.sleep(1)
        self.conn.close()
