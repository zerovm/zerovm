#!/bin/sh

for i in `ls *.manifest`; do
  $ZEROVM_ROOT/zerovm $i > /dev/null
  if [ $? = 0 ]; then
    echo "\033[01;38m$i\033[00m test has \033[02;31mfailed\033[00m"
  else
    echo "\033[01;38m$i\033[00m test has \033[01;32mpassed\033[00m"
  fi
done
