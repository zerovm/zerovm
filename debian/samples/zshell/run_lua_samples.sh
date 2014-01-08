#!/bin/bash

echo Run lua samples

./genmanifest.sh lua/scripts/hello.lua log/lua1.stdout /dev/null /dev/null > lua/hello_lua.manifest 
echo -------------------------------run lua #1: hello world
rm log/lua1.stdout -f
echo zerovm -Mlua/hello_lua.manifest
zerovm -Mlua/hello_lua.manifest
echo "stdout output >>>>>>>>>>"
cat log/lua1.stdout

./genmanifest.sh lua/scripts/pngparse.lua log/lua2.stdout lua/scripts/280x.png log/lua2.stderr.log "/dev/input" > lua/pngparse_lua.manifest 
echo -------------------------------run lua #2: png parse
rm log/lua2.stdout -f
echo zerovm -v4 -Mlua/pngparse_lua.manifest
zerovm -v4 -Mlua/pngparse_lua.manifest
echo "stderr output >>>>>>>>>>"
cat log/lua2.stdout

./genmanifest.sh lua/scripts/command_line.lua log/lua3.stdout /dev/null log/lua3.stderr.log "var1 var2 var3"  > lua/command_line_lua.manifest 
echo -------------------------------run lua #3: command line
rm log/lua3.stdout -f
echo zerovm -Mlua/command_line_lua.manifest
zerovm -Mlua/command_line_lua.manifest
echo "stderr output >>>>>>>>>>"
cat log/lua3.stdout

