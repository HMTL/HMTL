import Queue


class CircularBuffer:
    """
    This class presents a circular buffer that can be used to
    consume and watch data from a stream
    """

    def __init__(self, limit):
        self.limit = limit
        self.queue = Queue.Queue(limit)

    def put(self, elem):
        while True:
            try:
                self.queue.put(elem, False)
                return
            except Queue.Full as e:
                self.queue.get(False)

    def get(self, block=True, wait=3600):
        try:
            return self.queue.get(block, wait)
        except Queue.Empty:
            return None

    def clear(self):
        try:
            while self.queue.get(False):
                pass
        except Queue.Empty as e:
            pass

    #
    # Iteration
    #
    def __iter__(self):
        return self

    def next(self):
        try:
            return self.queue.get(False)
        except Queue.Empty as e:
            raise StopIteration

    #
    # len()
    #
    def __len__(self):
        return self.queue.qsize()