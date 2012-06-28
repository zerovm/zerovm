#!/bin/bash

cd ../..

#ALL 10SRC, 10DST
#SRC_NODES_LIST="2 3 4 5 6 7 8 9 10 11"
#DST_NODES_LIST="12 13 14 15 16 17 18 19 20 21"

#config for 5src, 5dst nodes
SRC_FIRST=2
SRC_LAST=11
DST_FIRST=12
DST_LAST=21

rm -f ./samples/disort/time

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
	gnome-terminal --geometry=80x20 -t "zerovm sortsrc$COUNTER.manifest" -x sh -c "./zerovm -Msamples/disort/manifest/sortsrc"$COUNTER".manifest"
    let COUNTER=COUNTER+1 
done

COUNTER=$DST_FIRST
while [  $COUNTER -le $DST_LAST ]; do
    gnome-terminal --geometry=80x20 -t "zerovm sortdst$COUNTER.manifest" -x sh -c "./zerovm -Msamples/disort/manifest/sortdst"$COUNTER".manifest"
    let COUNTER=COUNTER+1 
done

date > ./samples/disort/time
gnome-terminal --geometry=80x20 -t "zerovm sortman.manifest" -x sh -c "./zerovm -Msamples/disort/manifest/sortman.manifest; date >> ./samples/disort/time"

cd samples/disort
cat log/sortman.stderr.log
echo --------------------------
cat time

