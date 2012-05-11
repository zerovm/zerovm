INCLUDE_FLAGS=-I ~/git/zerovm 
MACROS_FLAGS=-D USER_SIDE
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain

all:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o zvm.o -I . \
	-Wall -c -Wno-long-long -O2 -msse4.1 -m64 zvm.c
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -c syscall_manager.S $(DEBUG_ENABLE)
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o zrt.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG_ENABLE) \
	-c -Wno-long-long -pthread -O2 -msse4.1 -m64 zrt.c

clean:	
	rm -f *.o
