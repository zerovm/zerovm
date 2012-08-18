INCLUDE_FLAGS=-I ..
MACROS_FLAGS=-D USER_SIDE
NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all:
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o zvm.o -I . \
	-Wall -c -Wno-long-long -O2 -msse4.1 -m64 zvm.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c syscall_manager.S $(DEBUG_ENABLE)
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o zrt.o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG_ENABLE) \
	-c -Wno-long-long -pthread -O2 -msse4.1 -m64 zrt.c
	@ar rc libzrt.a syscall_manager.o zvm.o zrt.o
	@echo ZeroVM API library successfully compiled to "libzrt.a"

clean:	
	@rm -f *.o *.a
	@echo ZeroVM API library deleted
