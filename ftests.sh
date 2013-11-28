#!/bin/bash

errors=0

# Verify that the ZEROVM_ROOT var is set:
if [ -z "${ZEROVM_ROOT}" ]; then
    echo "Error: ZEROVM_ROOT variable is not set"
    exit 1
fi

cd $ZEROVM_ROOT/tests/functional/include
make -s clean all

for i in $(find $ZEROVM_ROOT/tests -mindepth 1 -type d); do
    cd $i
    if [ -x ./test.sh ]; then
        output=$(./test.sh | tee /dev/tty)
        # check for and count failures
        if [[ "$?" -ne 0 || $(echo "$output" | grep "failed") ]]; then
            errors=$(($errors + 1))
        fi
    fi
done

if [ $errors -gt 0 ]; then
    # If any of the tests failed,
    # make sure we don't falsely return a 0 exit status
    echo "There were $errors errors"
    exit 1
fi
