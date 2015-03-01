#!/usr/bin/python
#
#export PYTHONPATH=$PYTHONPATH:/Users/amp/Dropbox/Myo/myo-python/
#export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:/Users/amp/Dropbox/Myo/MyoSDK/myo.framework"

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))
from optparse import OptionParser, OptionGroup


import myo
from myo.lowlevel import pose_t, stream_emg
from myo.six import print_
import random

from client import *
import HMTLprotocol


myo.init()

class Listener(myo.DeviceListener):
    # return False from any method to stop the Hub

    def __init__(self, client, options):
        self.client = client
        self.options = options

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

        msg = None
        if pose == pose_t.double_tap:
            msg = HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                           1,
                                           255,0,255)
        elif pose == pose_t.rest:
            msg = HMTLprotocol.get_value_msg(self.options.hmtladdress,
                                             1,
                                             0)
        elif pose == pose_t.wave_in:
            msg = HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                           1,
                                           0,255,0)
        elif pose == pose_t.wave_out:
            msg = HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                           1,
                                           0,0,255)
        elif pose == pose_t.fist:
            msg = HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                           1,
                                           255,255,255)
        elif pose == pose_t.fingers_spread:
            msg = HMTLprotocol.get_rgb_msg(self.options.hmtladdress,
                                           1,
                                           255,0,0)

        if (msg != None):
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
