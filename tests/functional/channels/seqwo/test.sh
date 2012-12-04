#!/bin/sh

name="sequential write only channel"

test="\033[01;38m$name\033[00m test has"
make clean all>/dev/null
result=$(grep "FAILED" result.log | awk '{print $4}')
if [ "" = "$result" ] && [ -f result.log ]; then
        echo "$test \033[01;32mpassed\033[00m"
        make clean>/dev/null
else
        echo "$test \033[01;31mfailed with $result errors\033[00m"
fi
