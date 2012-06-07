#!/bin/bash

#ALL 50SRC, 50DST
#SRC_NODES_LIST="2 3 4 5 6 7 8 9 10 11"
#DST_NODES_LIST="12 13 14 15 16 17 18 19 20 21"

#Generate from template 10src, 10dst manifest files
SRC_FIRST=2
SRC_LAST=11
DST_FIRST=12
DST_LAST=21

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
	#cp manifest/sortsrc.manifest.template manifest/sortsrc"$COUNTER".manifest
    sed s/{NODEID}/$COUNTER/ manifest/sortsrc.manifest.template > manifest/sortsrc"$COUNTER".manifest
    let COUNTER=COUNTER+1 
done

COUNTER=$DST_FIRST
while [  $COUNTER -le $DST_LAST ]; do
	#cp manifest/sortdst.manifest.template manifest/sortdst"$COUNTER".manifest
    sed s/{NODEID}/$COUNTER/ manifest/sortdst.manifest.template > manifest/sortdst"$COUNTER".manifest
    let COUNTER=COUNTER+1 
done


