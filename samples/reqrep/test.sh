#!/bin/bash

gnome-terminal --geometry=80x20 -t "../../zerovm test1.manifest" -x sh -c "../../zerovm -Mtest1.manifest -v10"
./zerovm -M../samples/reqrep/test2.manifest

sleep 1
echo "############### test 1 #################"
cat log/test1.stderr.log
echo "############### test 2 #################"
cat log/test2.stderr.log



