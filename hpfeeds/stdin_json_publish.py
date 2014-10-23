import sys
import json
import select
import traceback
import logging
import socket
import time

logging.basicConfig(level=logging.CRITICAL)

import hpfeeds

HOST = 'localhost'
PORT = 10000
CHANNELS = []
IDENT = ''
SECRET = ''

if len(sys.argv) > 1:
    print >>sys.stderr, "Parsing config file: %s"%sys.argv[1]
    config = json.load(file(sys.argv[1]))
    HOST        = config["HOST"]
    PORT        = config["PORT"]
    # hpfeeds protocol has trouble with unicode, hence the utf-8 encoding here
    PUB_CHANNEL = config["PUB_CHANNEL"].encode("utf-8")
    IDENT       = config["IDENT"].encode("utf-8")
    SECRET      = config["SECRET"].encode("utf-8")
else:
    print >>sys.stderr, "Usage: %s <config.json>"%sys.argv[0]
    print >>sys.stderr, '''
Example config.json:
    {
        "HOST": "localhost",
        "PORT": 10000,
        "IDENT": "geoloc", 
        "SECRET": "secret",
        "PUB_CHANNEL": "wordpot.events"
    }
'''

    sys.exit(1)

def main():
    hpc = hpfeeds.new(HOST, PORT, IDENT, SECRET)
    print >>sys.stderr, 'connected to', hpc.brokername

    def on_message(identifier, channel, payload):
        print >>sys.stderr, ' -> message from server (and there shouldnt be): {0}'.format(payload)
        sys.exit(1)

    def on_error(payload):
        print >>sys.stderr, ' -> errormessage from server: {0}'.format(payload)
        sys.exit(1)

    hpc.s.settimeout(0.01)

    while True:
        rrdy, wrdy, xrdy = select.select([hpc.s, sys.stdin], [], [])
        if hpc.s in rrdy:
            try: 
                print >>sys.stderr, 'hpc.run...'
                hpc.run(on_message, on_error)
            except socket.timeout: 
                pass

        line = sys.stdin.readline().strip()
        if line:
            try:
                # ensure the message is json
                json.loads(line) 
            except:
                traceback.print_exc()
            else:
                hpc.publish([PUB_CHANNEL,], line)
        else:
            time.sleep(0.1)

    print >>sys.stderr, 'Exitting.'
    hpc.close()
    return 0

if __name__ == '__main__':
    try: 
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(0)
