~/nacl/native_client/toolchain/linux_x86/bin/x86_64-nacl-gcc -o killme.o -DUSER_SIDE -Wall -c -Wno-long-long -O2 -msse4.1 -m64 -iquote/home/dazo/git/zerovm -iquote/home/dazo/git/zerovm/api -I/home/dazo/nacl/native_client/toolchain/linux_x86/x86_64-nacl/include $1

#### compile asm source (handler)
~/nacl/native_client/toolchain/linux_x86/bin/x86_64-nacl-gcc -c syscall_manager.S

~/nacl/native_client/toolchain/linux_x86/bin/x86_64-nacl-gcc -o $1_x86_64.nexe -s -static -T ~/nacl/native_client/toolchain/linux_x86/x86_64-nacl/lib/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 killme.o ~/git/zerovm/api/zvm.o syscall_manager.o

rm killme.o syscall_manager.o

