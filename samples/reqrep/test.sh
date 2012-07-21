#!/bin/bash

touch testa
touch testb

cd ../..

gnome-terminal --geometry=80x20 -t "zerovm test1.manifest" -x sh -c "./zerovm -Msamples/reqrep/test1.manifest -v10"
./zerovm -Msamples/reqrep/test2.manifest

sleep 1
echo "############### test 1 #################"
cat samples/reqrep/log/test1.stderr.log
echo "############### test 2 #################"
cat samples/reqrep/log/test2.stderr.log



