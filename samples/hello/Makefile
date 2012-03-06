INCLUDE_FLAGS=-I ~/git/zerovm 
MACROS_FLAGS=-D USER_SIDE
OUTPUT_FILE=hello_world.c_x86_64.nexe
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain

all: hello_world.o
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o $(OUTPUT_FILE) -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/linux_x86/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) hello_world.o ~/git/zerovm/api/zvm.o

hello_world.o:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o hello_world.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -pthread -O2 -msse4.1 -m64 hello_world.c

clean:	
	rm -f *.nexe
	rm -f *.o