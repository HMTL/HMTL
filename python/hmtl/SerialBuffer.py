import threading
import serial
import time

from CircularBuffer import CircularBuffer


class SerialBuffer(threading.Thread):
    """
    This class reads from a serial port into a circular buffer
    """

    def __init__(self, device, baud=9600, timeout=10, bufflen=1000):
        threading.Thread.__init__(self)

        self.buff = CircularBuffer(bufflen)

        self.connection = serial.Serial(device, baud, timeout=timeout)

        # Set as a daemon so that this thread will exit correctly
        # when the parent receives a kill signal
        self.daemon = True

    def get_buffer(self):
        return self.buff

    def stop(self):
        self._Thread__stop()

    def run(self):
        while True:
            data = self.connection.readline()

            if data and len(data) > 0:
                item = SerialItem(data, time.time())
                self.buff.put(item)


class SerialItem:
    """
    Class containing data from a single serial item
    """

    def __init__(self, data, timestamp):
        self.data = data
        self.timestamp = timestamp