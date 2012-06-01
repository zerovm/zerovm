#!/bin/bash
SRC_NODES_LIST="2 3 4 5 6"

mkdir data -p

for number in $SRC_NODES_LIST
do
	FILE_NAME=data/"$number"unsorted.data;
	if [ ! -f $FILE_NAME ]
		then
		echo Creating file $FILE_NAME
		dd if=/dev/urandom of=$FILE_NAME bs=4000 count=1000
	fi
done

