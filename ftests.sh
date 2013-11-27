#!/bin/bash

errors=0

# Verify that the ZEROVM_ROOT var is set:
env | grep -q ^ZEROVM_ROOT=
if [ $? -ne 0 ]; then
    echo "Error: ZEROVM_ROOT variable is not set"
    exit 1
fi

cd $ZEROVM_ROOT/tests/functional/include
make -s clean all

for i in `find $ZEROVM_ROOT/tests -type d`; do
  if [ `pwd` = $i ]; then
    continue
  fi
    cd $i
    if [ -f ./test.sh ]; then
        output=`./test.sh`
        # check for and count failures
        if [[ $? -eq 1 || `echo "$output" | grep "failed"` ]]; then
            errors=`expr $errors + 1`
        fi
        printf "$output\n"
    fi
done

if [ $errors -gt 0 ]; then
    # If any of the tests failed,
    # make sure we don't falsely return a 0 exit status
    echo "There were $errors errors"
    exit 1
fi
