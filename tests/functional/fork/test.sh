#!/bin/sh

printf "\033[01;38mfork\033[00m test has"
make clean all > /dev/null
python daemon_client.py fork_test < forked.manifest >> LOG

printf "\00\00\00\00\00\00\00\00\00\00\00\00\00\00after fork()\n" > forked_err.ctrl
printf "stdout: after fork()\n" > forked_out.ctrl
printf "before fork()\n" > fork_err.ctrl
printf "" > fork_out.ctrl

cmp -s forked_err.ctrl forked_err.log 2> /dev/null
if [ "0" != "$?" ]; then
  echo " \033[01;31mfailed\033[00m on 1"
  exit 1
fi

cmp -s forked_out.ctrl forked_out.log 2> /dev/null
if [ "0" != "$?" ]; then
  echo " \033[01;31mfailed\033[00m on 2"
  exit 2
fi

cmp -s fork_err.ctrl fork_err.log 2> /dev/null
if [ "0" != "$?" ]; then
  echo " \033[01;31mfailed\033[00m on 3"
  exit 3
fi

cmp -s fork_out.ctrl fork_out.log 2> /dev/null
if [ "0" != "$?" ]; then
  echo " \033[01;31mfailed\033[00m on 4"
  exit 4
fi

make clean > /dev/null
echo " \033[01;32mpassed\033[00m"
exit 0

