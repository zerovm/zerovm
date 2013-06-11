#!/bin/sh

printf "\033[01;38mnet copy\033[00m test has"

make clean all>/dev/null
result=$(cmp output.data input.data | grep " ")
if [ "" != "$result" ]; then
        echo " \033[01;31mfailed\033[00m"
else
        echo " \033[01;32mpassed\033[00m"
        make clean>/dev/null
fi
