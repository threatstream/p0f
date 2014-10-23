#!/bin/bash

set -e

DIR=`dirname $0`

INTERFACE=eth0
MY_ADDRESS=`ifconfig ${INTERFACE} | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'`

# only sniff on incoming traffic, not outbound
FILTER="!(src host ${MY_ADDRESS})"

if [ -z "$HPFEEDS_HOST" ] 
then
    echo 'HPFEEDS_HOST is not set'
    exit 1
fi

if [ -z "$HPFEEDS_PORT" ] 
then
    echo 'HPFEEDS_PORT is not set'
    exit 1
fi

if [ -z "$HPFEEDS_CHANNEL" ] 
then
    echo 'HPFEEDS_CHANNEL is not set'
    exit 1
fi

if [ -z "$HPFEEDS_SECRET" ] 
then
    echo 'HPFEEDS_SECRET is not set'
    exit 1
fi

if [ -z "$HPFEEDS_IDENT" ] 
then
    echo 'HPFEEDS_IDENT is not set'
    exit 1
fi

OPTS="-i ${INTERFACE} -u p0f-user"
OPTS+=" -h -H ${HPFEEDS_HOST} -P ${HPFEEDS_PORT}"
OPTS+=" -I ${HPFEEDS_IDENT} -K ${HPFEEDS_SECRET} -C ${HPFEEDS_CHANNEL}"

exec ${DIR}/p0f $OPTS "${FILTER}" 
