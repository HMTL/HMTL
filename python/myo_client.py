#!/usr/bin/python
#
# In order to run, the Myo SDK and the python-myo library must have their paths
# added:
#   export PYTHONPATH=$PYTHONPATH:/Users/amp/Dropbox/Myo/myo-python/
#   export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:/Users/amp/Dropbox/Myo/MyoSDK/myo.framework"

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))
from optparse import OptionParser, OptionGroup
from time import time

import myo
from myo.lowlevel import pose_t, stream_emg
from myo.six import print_
import random

from client import *
import HMTLprotocol


myo.init()

class prettyfloat(float):
    def __repr__(self):
        return "%6.2f" % self

def pf(data):
    return map(prettyfloat, data)

class Listener(myo.DeviceListener):
    # return False from any method to stop the Hub
    OUTPUT_PERIOD = 0.25

    def __init__(self, client, options):
        self.client = client
        self.options = options
        self.last_output = 0
        self.data = {}
        self.data["gyroscope"] = {"data": []}
        self.data["acceleration"] = {"data": []}
        self.data["orientation"] = {"data": []}

    def on_connect(self, myo, timestamp):
        print_("Connected to Myo")
        myo.vibrate('short')
        myo.request_rssi()

    def on_rssi(self, myo, timestamp, rssi):
        print_("RSSI:", rssi)

    def on_event(self, event):
        r""" Called before any of the event callbacks. """

    def on_event_finished(self, event):
        r""" Called after the respective event callbacks have been
        invoked. This method is *always* triggered, even if one of
        the callbacks requested the stop of the Hub. """

    def on_pair(self, myo, timestamp):
        print_('Paired')
        print_("If you don't see any responses to your movements, try re-running the program or making sure the Myo works with Myo Connect (from Thalmic Labs).")
        print_("Double tap enables EMG.")
        print_("Spreading fingers disables EMG.\n")

    def on_disconnect(self, myo, timestamp):
        print_('on_disconnect')

    def on_pose(self, myo, timestamp, pose):
        print_('on_pose', pose)

        self.data["on_pose"] = { "timestamp" : timestamp, "data" : pose }

        msgs = []
        if pose == pose_t.double_tap:
            msgs += [ HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                              1,
                                              255,0,255) ]
        elif pose == pose_t.rest:
            msgs += [
                HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                         1,
                                         0,0,0),
                
                HMTLprotocol.get_value_msg(69,
                                           0,
                                           0),

                HMTLprotocol.get_value_msg(69,
                                           1,
                                           0)
            ]

        elif pose == pose_t.wave_in:
            msgs += [ HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                               1,
                                               0,255,0) ]
        elif pose == pose_t.wave_out:
            msgs += [ HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                               1,
                                               0,0,255) ]
        elif pose == pose_t.fist:
            msgs += [
                HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                         1,
                                         255,0,0),
                HMTLprotocol.get_value_msg(69,
                                           1,
                                           255) 
            ]
        elif pose == pose_t.fingers_spread:
            msgs += [ 
                HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                         1,
                                         255,255,255),
                HMTLprotocol.get_value_msg(69,
                                           0,
                                           128) 
                ]

        if (len(msgs) != 0):
            for msg in msgs:
                self.client.send_and_ack(msg, False)
            print("Sent and acked to client")

    def on_unlock(self, myo, timestamp):
        print_('unlocked')

    def on_lock(self, myo, timestamp):
        print_('locked')

    def on_sync(self, myo, timestamp, arm, x_direction):
        print_('synced', arm, x_direction)

    def on_unsync(self, myo, timestamp):
        print_('unsynced')

    def on_orientation_data(self, myo, timestamp, orientation):
        self.data["orientation"] = { "timestamp" : timestamp, "data" : orientation }
        self.update_output()

    def on_accelerometor_data(self, myo, timestamp, acceleration):
        self.data["acceleration"] = { "timestamp" : timestamp, "data" : acceleration }
        self.update_output()

    def on_gyroscope_data(self, myo, timestamp, gyroscope):
        self.data["gyroscope"] = { "timestamp" : timestamp, "data" : gyroscope }
        self.update_output()

    def update_output(self):
        if (time.time() - self.last_output > self.OUTPUT_PERIOD):
            print("O%s A%s G%s" % (
                pf(self.data["orientation"]["data"]),
                pf(self.data["acceleration"]["data"]),
                pf(self.data["gyroscope"]["data"]),
            ))
            self.last_output = time.time()

def handle_args():
    global options

    parser = OptionParser()

    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")
    # General options
    parser.add_option("-A", "--hmtladdress", dest="hmtladdress", type="int",
                      help="Address to which messages are sent [default=BROADCAST]",
                      default=HMTLprotocol.BROADCAST)
    (options, args) = parser.parse_args()

    return options

def main():
    options = handle_args()

    print("%s\nOpening client.  Address=%d\n%s" % ("*"*80, options.hmtladdress, "*"*80))

    client = HMTLClient(options)

    hub = myo.Hub()
    hub.set_locking_policy(myo.locking_policy.none)
    hub.run(1000, Listener(client, options))

    # Listen to keyboard interrupts and stop the
    # hub in that case.
    try:
        while hub.running:
            myo.time.sleep(0.2)
    except KeyboardInterrupt:
        print_("Quitting ...")
        hub.stop(True)

if __name__ == '__main__':
    main()
