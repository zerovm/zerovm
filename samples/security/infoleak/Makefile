NAME=test_infoleak
INCLUDE_FLAGS=-I ~/git/zerovm
# note: NACL_sys_test_infoleak was defined in old nacl_syscalls.h (sel_ldr). was removed with 
# most of the other syscalls. now we need to find substitution for this test
MACROS_FLAGS=-D USER_SIDE -DNACL_BLOCK_SHIFT=5 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_sys_test_infoleak=109
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain/linux_x86


all: $(NAME).o
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o ~/git/zerovm/api/zvm.o

$(NAME).o:
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	rm -f $(NAME).nexe $(NAME).o *.log
