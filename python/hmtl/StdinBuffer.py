#
# This class reads from a STDIN into a circular message buffer.  The
# data read can either be line terminated ('\n') or as HMTL messages
#
# Author: Adam Phelps
# License: MIT
# Copyright: 2015
#

from __future__ import print_function

import sys

from InputBuffer import InputBuffer


class StdinBuffer(InputBuffer):
    """
    This class reads from STDIN into a circular buffer.
    """

    def __init__(self, bufflen=1000, verbose=True):
        InputBuffer.__init__(self, bufflen, verbose)

    def get_reader(self):
        return sys.stdin
