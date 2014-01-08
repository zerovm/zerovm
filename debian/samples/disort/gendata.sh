#!/bin/bash

mkdir data -p

SRC_FIRST=1
SRC_LAST=10

COUNTER=$SRC_FIRST
while [  $COUNTER -le $SRC_LAST ]; do
    echo generate data for $COUNTER node
    zerovm -Mmanifest/generator"$COUNTER".manifest -v1
    cat log/"$COUNTER"generator.stderr.log
    let COUNTER=COUNTER+1 
done
