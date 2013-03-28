#!/bin/sh

printf "\033[01;38mfull random access (random read, random write) channel\033[00m test has"
make clean all>/dev/null
result=$(grep "OVERALL TEST FAILED" result.log | awk '{print $5}')
if [ "" = "$result" ] && [ -s result.log ]; then
        echo " \033[01;32mpassed\033[00m"
        make clean>/dev/null
else
        echo " \033[01;31mfailed with $result errors\033[00m"
fi
