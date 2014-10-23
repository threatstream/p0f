#!/bin/bash

set -e

trap_with_arg() {
    func="$1" ; shift
    for sig ; do
        trap "$func $sig" "$sig"
    done
}

_cleanup() {
    if [ -z "$_cleanupInitiated" ]; then
        _cleanupInitiated=true
        echo "info: signal trapped, signal was: $1"
        # Kill components
        for pid in $P0F_PID $TAIL_PID $AGG_PID $PUB_PID; 
        do
            if [ -n "$pid" ];
            then
                echo kill -9 $pid
                kill -9 $pid 1>&2 2>/dev/null || echo "failed to kill $pid"
            fi
        done
        echo 'goodbye.'
        exit 0
    else
        echo 'info: cleanup already started'
    fi
}

trap_with_arg _cleanup INT TERM EXIT

DIR=`dirname $0`

INTERFACE=eth0
MY_ADDRESS=`ifconfig ${INTERFACE} | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'`

# only sniff on incoming traffic, not outbound
FILTER="!(src host ${MY_ADDRESS})"

${DIR}/p0f -i ${INTERFACE} -o /var/log/p0f.json.log -u p0f-user -j -l "${FILTER}" > /dev/null &
P0F_PID=$!

#---

HPFEEDS_DIR=$DIR/hpfeeds
PYTHON=${HPFEEDS_DIR}/env/bin/python
CONFIG=${HPFEEDS_DIR}/p0f_hpfeeds.json
LOG=/var/log/p0f.json.log

tail -n 0 -F ${LOG} | ${PYTHON} -u ${HPFEEDS_DIR}/aggregating_filter.py | ${PYTHON} -u ${HPFEEDS_DIR}/stdin_json_publish.py ${CONFIG} &
TAIL_PID=`pgrep -f "tail -n 0 -F ${LOG}"`
AGG_PID=`pgrep -f "python -u ${HPFEEDS_DIR}/aggregating_filter.py"`
PUB_PID=`pgrep -f "python -u ${HPFEEDS_DIR}/stdin_json_publish.py ${CONFIG}"`

echo "P0F_PID=$P0F_PID, TAIL_PID=$TAIL_PID, AGG_PID=$AGG_PID, PUB_PID=$PUB_PID"

while true;
do 
	for PID in $P0F_PID  $TAIL_PID  $AGG_PID  $PUB_PID;
	do
		if ! kill -0 $PID 2> /dev/null;
		then
			echo "Process died: $PID"
			exit 1
		fi
	done
	sleep 1;
done
exit 1

