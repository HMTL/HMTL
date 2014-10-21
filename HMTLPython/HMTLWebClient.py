#!/usr/bin/python
#
# Web form for sending commands to an HMTL Server

from __future__ import print_function
from __future__ import unicode_literals

import sys, os
sys.path.append(os.path.join(os.path.dirname(__file__), "lib"))

from wsgiref.simple_server import make_server
from io import StringIO
import re
from optparse import OptionParser

from client import *
import HMTLprotocol

bind_address=''
bind_port=8000

triggers = []
values = [0, 64, 128, 196, 255]

trig_state = {
    0: 0,
    1: 0,
    2: 0,
    3: 0
}

def handle_args():
    global options

    parser = OptionParser()

    # Client options
    parser.add_option("-r", "--rgb", dest="setrgb", action="store_true",
                      help="Set RGB value", default=False)
    parser.add_option("-P", "--period", dest="period", type="float",
                      help="Sleep period between changes")
    parser.add_option("-A", "--hmtladdress", dest="hmtladdress", type="int",
                      help="Address to which messages are sent")

    # General options
    parser.add_option("-v", "--verbose", dest="verbose", action="store_true",
                      help="Verbose output", default=False)
    parser.add_option("-p", "--port", dest="port", type="int",
                      help="Port to bind to", default=6000)
    parser.add_option("-a", "--address", dest="address",
                      help="Address to bind to", default="localhost")


    (options, args) = parser.parse_args()
    print("options:" + str(options) + " args:" + str(args))

    return (options, args)


#
# Return a trigger and value from 'trig=<trig>&value=<val>'
#
trig_matcher = re.compile('trig=([0-9]+)')
value_matcher = re.compile('value=([0-9]+)')
def get_trig_value(str_data):
    trigmatch = trig_matcher.search(str_data)
    valmatch = value_matcher.search(str_data)
    if (trigmatch and valmatch):
        trig = int(trigmatch.group(1))
        val = int(valmatch.group(1))
        return (trig, val)
    return (None, None)


def send_update(trig, val):
    trig_state[trig] = val

    print("Sending: trig=%d val=%d" % (trig, val))

    msg = HMTLprotocol.get_value_msg(HMTLprotocol.BROADCAST, trig, val)
    client.send_and_ack(msg)
    

#
# Parse form data
#
def handle_post(post_data):
    str_data = post_data.decode()
    print("Received post data '%s'" % (str_data))
    (trig, val) = get_trig_value(str_data)
    if ((trig != None) and (val != None)):
        print("Trigger=%d Value=%d" % (trig, val))
        send_update(trig, val)
    
#
# Generate the form
#

header = "<html><header> <title>Trigger Board Control</title></header><body>"
footer = "</body></html>"
def html_page(content):
    page = "%s\n%s\n%s" % (header, content, footer)
    return page.encode()

def form_app(environ, start_response):
    status = str('200 OK')
#    headers = [('Content-Type', 'text/hmtl; charset=utf-8')] # Don't know???
    headers=[(str('Content-Type'), str('text/html; charset=utf-8'))]
    start_response(status, headers)

    output = StringIO()
    print("<H1>This sends commands to Adam's trigger board</H1>", file=output)
    if environ['REQUEST_METHOD'] == 'POST':
        size = int(environ['CONTENT_LENGTH'])
        post_str = environ['wsgi.input'].read(size)
        handle_post(post_str)

    for trig in trig_state:
        print("Trigger %d: %d<p>" % (trig, trig_state[trig]), file=output)

    print('<form method="POST">', file=output)

    print("triggers=%s" % (triggers))

    print('Trigger:<select name=trig>',
          "".join(['<option value="%d">%d</option>' % (t, t) for t in triggers]),
          '</select>',
          file=output)

    print('Value:<select name=value>',
          "".join(['<option value="%d">%d</option>' % (t, t) for t in values]),
          '</select>',
          file=output)

    print('<input type="submit" value="Send"></form>', file=output)

    return [html_page(output.getvalue())]

def main():
    global triggers
    global client

    handle_args()

    triggers = list(trig_state.keys())
    print("triggers=%s" % (triggers))

    client = HMTLClient(options)

    httpd = make_server(bind_address, bind_port, form_app)
    print("Serving on port %s:%d" % (bind_address, bind_port))

    httpd.serve_forever()
    client.close()

main()
