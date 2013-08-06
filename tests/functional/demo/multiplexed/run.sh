#!/bin/bash
echo hello1
/home/bortoq/git/zerovm/zerovm hello1.manifest&
echo hello2
/home/bortoq/git/zerovm/zerovm hello2.manifest&
echo hello3
/home/bortoq/git/zerovm/zerovm hello3.manifest&
echo copy
/home/bortoq/git/zerovm/zerovm copy.manifest
cat output.data
rm output.data
