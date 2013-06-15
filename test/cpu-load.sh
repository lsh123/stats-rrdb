#!/bin/sh

HOST=127.0.0.1
PORT=9876
SLEEP=1

while [ 1 ] ;
do
	date
	CURRENT_LOAD=`cat /proc/loadavg | sed 's/ .*//'`
	CURRENT_TS=`date -u +%s`

	echo -n "u|system.cpu.load|$CURRENT_LOAD|$CURRENT_TS" | nc -4u -w0 $HOST $PORT
	#echo -n "UPDATE 'system.cpu.load' ADD $CURRENT_LOAD AT $CURRENT_TS ;" | nc -4t -w1 $HOST $PORT
	
	echo
	sleep $SLEEP
done
