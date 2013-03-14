#!/bin/sh

name="net copy"

test="\033[01;38m$name\033[00m test has"
make clean all>/dev/null
result=$(cmp output.data input.data | grep " ")
if [ "" != "$result" ]; then
        echo "$test \033[01;31mfailed\033[00m"
else
        echo "$test \033[01;32mpassed\033[00m"
        make clean>/dev/null
fi
