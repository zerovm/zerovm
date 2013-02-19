NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: zvm.c
	@$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 -I. $^
	@$(ZVM_TOOLCHAIN)/bin/ar rc libzvm.a zvm.o
	@echo ZeroVM API library successfully compiled to "libzvm.a"

clean:
	@rm -f *.o *.a
	@echo ZeroVM API library deleted
