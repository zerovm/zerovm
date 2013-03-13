#!/bin/sh

if [ ! -s $ZEROVM_ROOT/tests/functional/include/libzvmlib.a ]; then
  cd $ZEROVM_ROOT/tests/functional/include
  make clean all
fi

for i in `find $ZEROVM_ROOT/tests -type d`; do
  if [ `pwd` = $i ]; then
    continue
  fi
    cd $i
    if [ -f ./test.sh ]; then
        ./test.sh
    fi
done
