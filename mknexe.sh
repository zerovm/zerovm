~/nacl/native_client/toolchain/linux_x86/bin/x86_64-nacl-gcc -o killme.o -Wall -c -Wno-long-long -pthread -O2 -msse4.1 -m64 -I~/sandbox/ -I ~/nacl/native_client/toolchain/linux_x86/x86_64-nacl/include $1
~/nacl/native_client/toolchain/linux_x86/bin/x86_64-nacl-gcc -o $1_x86_64.nexe -static -T ~/nacl/native_client/toolchain/linux_x86/x86_64-nacl/lib/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 killme.o -lc -lpthread
rm killme.o

