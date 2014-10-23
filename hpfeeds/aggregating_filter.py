#!/usr/bin/python

import ujson as json
import sys
import cachetools

cache = cachetools.LRUCache(maxsize=1000)
NAMES = ['app','client_ip','dist','lang','link','mod','os','params','raw_hits','raw_mtu','raw_sig','reason','server_ip','subject', ]

def trunc_ts(timestamp):
    # input: "2014/10/07 18:04:10"
    # output: "2014/10/07 18:04"
    return timestamp[:16]

def get_key(rec):
    values = [unicode(rec.get(name, '')) for name in NAMES]
    values.append(trunc_ts(rec['timestamp']))
    return u'\t'.join(values)

for line in sys.stdin:
    rec = json.loads(line)
    key = get_key(rec)
    if key not in cache:
        cache[key] = 1
        sys.stdout.write(line)
sys.exit(1)
