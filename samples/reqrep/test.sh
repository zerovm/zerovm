#!/bin/bash

gnome-terminal --geometry=80x20 -t "zerovm test2.manifest" -x sh -c "../../zvm/zerovm -Mtest2.manifest -v10"
../../zvm/zerovm -Mtest1.manifest -v10

sleep 1
echo "############### test 1 #################"
cat log/1stderr.log
echo "############### test 2 #################"
cat log/2stderr.log



