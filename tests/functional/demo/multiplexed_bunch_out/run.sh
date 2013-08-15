#!/bin/bash
echo hello
/home/bortoq/git/zerovm/zerovm hello.manifest&
echo copy1
/home/bortoq/git/zerovm/zerovm copy1.manifest&
echo copy2
/home/bortoq/git/zerovm/zerovm copy2.manifest&
echo copy3
/home/bortoq/git/zerovm/zerovm copy3.manifest
cat output1.data output2.data output3.data 
rm output1.data output2.data output3.data 

