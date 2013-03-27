#!/bin/sh

cd $ZEROVM_ROOT/tests/functional/include
make -s clean all

for i in `find $ZEROVM_ROOT/tests -type d`; do
  if [ `pwd` = $i ]; then
    continue
  fi
    cd $i
    if [ -f ./test.sh ]; then
        echo $i
        ./test.sh
    fi
done
