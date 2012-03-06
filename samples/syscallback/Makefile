INCLUDE_FLAGS=-I ~/git/zerovm 
MACROS_FLAGS=-D USER_SIDE
OUTPUT_FILE1=syscallback_test.c_x86_64.nexe
OUTPUT_FILE2=zrt_mock.c_x86_64.nexe
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain

all: $(OUTPUT_FILE1) $(OUTPUT_FILE2)

$(OUTPUT_FILE1): syscallback_test.o
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o $(OUTPUT_FILE1) -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/linux_x86/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG_ENABLE) \
	syscallback_test.o ~/git/zerovm/api/zvm.o


$(OUTPUT_FILE2): zrt_mock.o syscall_manager.o
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o $(OUTPUT_FILE2) -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/linux_x86/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG_ENABLE) \
	zrt_mock.o syscall_manager.o ~/git/zerovm/api/zvm.o

syscall_manager.o:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -c syscall_manager.S $(DEBUG_ENABLE)

syscallback_test.o:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o syscallback_test.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG_ENABLE) \
	-c -Wno-long-long -pthread -O2 -msse4.1 -m64 syscallback_test.c

zrt_mock.o:
	$(NACL_TOOLCHAIN_PATH)/linux_x86/bin/x86_64-nacl-gcc -o zrt_mock.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG_ENABLE) \
	-c -Wno-long-long -pthread -O2 -msse4.1 -m64 zrt_mock.c


clean:	
	rm -f *.nexe
	rm -f *.o

cleanlog:
	rm -f *.log
