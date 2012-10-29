INCLUDE_FLAGS=-I ..
MACROS_FLAGS=-D USER_SIDE
NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all:
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o zvm.o -I . \
	-Wall -c -Wno-long-long -O3 -msse4.1 -m64 zvm.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-ar rc libzvm.a zvm.o
	@echo ZeroVM API library successfully compiled to "libzvm.a"

clean:
	@rm -f *.o *.a
	@echo ZeroVM API library deleted
