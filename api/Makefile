
all:
	~/nacl_sdk/pepper_16/toolchain/linux_x86/bin/x86_64-nacl-gcc -o zvm.o -I . \
	-Wall -c -Wno-long-long -O2 -msse4.1 -m64 zvm.c

clean:	
	rm -f *.o