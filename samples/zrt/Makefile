INCLUDE_FLAGS=-I ~/git/zerovm 
MACROS_FLAGS=-D USER_SIDE
OUTPUT_FILE=hello.nexe
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain

all: $(OUTPUT_FILE)

$(OUTPUT_FILE): hello.o
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o $(OUTPUT_FILE) -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/linux_x86/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG_ENABLE) \
	hello.o ~/git/zerovm/api/syscall_manager.o ~/git/zerovm/api/zrt.o ~/git/zerovm/api/zvm.o

hello.o:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o hello.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG_ENABLE) \
	-c -Wno-long-long -pthread -O2 -msse4.1 -m64 hello.c

clean:	
	rm -f hello.nexe hello.o *.log *.data

