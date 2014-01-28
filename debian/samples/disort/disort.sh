#!/bin/bash

./gendata.sh

../ns_start.sh 21

#config for 10src, 10dst nodes
SRC_FIRST=1
SRC_LAST=10
DST_FIRST=1
DST_LAST=10
GTERM=0

if [ -n "$(which gnome-terminal)" ]; then
	GTERM=1
fi

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
	if [[ "$GTERM" == "1" ]]; then
		gnome-terminal --geometry=80x20 -t "zerovm sortsrc$COUNTER.manifest" -x \
			sh -c "zerovm -Mmanifest/sortsrc"$COUNTER".manifest -e -v4"
	else
		echo "zerovm -Mmanifest/sortsrc"$COUNTER".manifest -e -v4"
		zerovm -Mmanifest/sortsrc"$COUNTER".manifest -e -v4 &
	fi
    let COUNTER=COUNTER+1 
done

COUNTER=$DST_FIRST
while [  $COUNTER -le $DST_LAST ]; do
	if [[ "$GTERM" == "1" ]]; then
    		gnome-terminal --geometry=80x20 -t "zerovm sortdst$COUNTER.manifest" -x \
			sh -c "zerovm -Mmanifest/sortdst"$COUNTER".manifest -e -v4"
	else
		echo "zerovm -Mmanifest/sortdst"$COUNTER".manifest -e -v4"
                zerovm -Mmanifest/sortdst"$COUNTER".manifest -e -v4 &
        fi
    let COUNTER=COUNTER+1 
done

date > /tmp/time
zerovm -Mmanifest/sortman.manifest -e -v4
date >> /tmp/time

cat log/sortman.stderr.log
echo Manager node working time: 
cat /tmp/time

../ns_stop.sh

