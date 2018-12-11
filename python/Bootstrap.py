#!/usr/bin/python
#
# Bootstrap script for a new device.  This uploads a configuration and installs
# the Bringup sketch.
#

import sys
import os
import argparse
import subprocess

import hmtl.portscan as portscan

def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config", dest="config",
                        required=True,
                        help="JSON configuration file")
    parser.add_argument("-d", "--device", dest="device",
                        help="Arduino USB device")
    parser.add_argument("-i", "--deviceid", dest="deviceid",
                        required=True,
                        help="Device ID to configure")
    parser.add_argument("-a", "--address", dest="address",
                        help="Address to configure (defaults to device ID)")
    parser.add_argument("-t", "--type", dest="type",
                        default="nano",
                        help="Device type for platformio scripts (nano, mini, uno, moteinomega, etc) [%(default)s)]")
    parser.add_argument("-s", "--stages", dest="stages",
                        default="1,2,3",
                        help="Stages to execute [%(default)s]")
    parser.add_argument("--module", dest="module",
                        default=False, action='store_true',
                        help="Initially load module code")

    options = parser.parse_args()

    if options.device == None:
        options.device = portscan.choose_port()

    return options


def main():
    options = parse_args()

    if not os.path.exists(options.config):
        print("Config file %s does not exist" % options.config)
        sys.exit(1)
    config_path = os.path.abspath(options.config)

    if not options.deviceid:
        print("Must specify a device ID")
        sys.exit(1)
    if not options.address:
        options.address = options.deviceid

    stages = [int(x) for x in options.stages.split(",")]

    platformio_cmd = ["platformio", "run", "-t", "upload", "-e",
                      "%s" % options.type]
    if 1 in stages:
        # Upload the python configuration sketch
        os.chdir("/Users/amp/Dropbox/Arduino/HMTL/platformio/HMTLPythonConfig")
        print("Executing: %s cwd:%s" % (platformio_cmd, os.getcwd()))
        ret = subprocess.call(platformio_cmd)
        if ret != 0:
            print("Uploading configuration sketch failed: %s" % ret)
            sys.exit(1)

    if 2 in stages:
        # Upload a configuration
        command = "HMTLConfig -f %s -i %s -a %s -v -w -d %s" % \
                  (config_path, options.deviceid, options.address, options.device)
        print("Executing: %s" % command)
        ret = os.system(command)
        #ret = subprocess.call(command)
        if ret != 0:
            print("HMTLConfig call failed: %s" % ret)
            sys.exit(1)

    if 3 in stages:
        # Upload the initial sketch
        if options.module:
            sketch="/Users/amp/Dropbox/Arduino/HMTL/platformio/HMTL_Module"
        else:
            sketch="/Users/amp/Dropbox/Arduino/HMTL/platformio/HMTL_Bringup"
        os.chdir(sketch)
        print("Executing: %s cwd:%s" % (platformio_cmd, os.getcwd()))
        ret = subprocess.call(platformio_cmd)
        if ret != 0:
            print("Uploading bringup sketch failed: %s" % ret)
            sys.exit(1)

main()