################################################################################
# Author: Adam Phelps
# License: MIT
# Copyright: 2015
#
# Logging class that prints a color timestamp before each log line
#
################################################################################

from __future__ import print_function

import time
import colorama
from colorama import Fore, Back, Style


class TimedLogger:
    """
    This class is used to log text with consistent timestamps and colors
    """

    RED = Fore.RED
    GREEN = Fore.GREEN
    BLUE = Fore.BLUE
    YELLOW = Fore.YELLOW
    MAGENTA = Fore.MAGENTA
    CYAN = Fore.CYAN
    WHITE = Fore.WHITE

    def __init__(self, start_time=None, textcolor=WHITE, timecolor=GREEN):
        if not start_time:
            self.start_time = time.time()
        else:
            self.start_time = start_time

        colorama.init()

        self.textcolor = textcolor
        self.timecolor = timecolor

        self.enabled = True

    def disable(self):
        self.enabled = False

    def enable(self):
        self.enabled = True

    def log(self, text, timestamp=None, color=None):
        if not timestamp:
            timestamp = time.time()
        if not color:
            color = self.textcolor

        if self.enabled:
            # Print the timestamp
            print(self.timecolor + "[%.3f] " % (timestamp - self.start_time), end="")

            # Print the message and reset the color
            print(color + text + Fore.RESET)