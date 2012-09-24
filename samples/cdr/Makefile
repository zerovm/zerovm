NAME=cdr
NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
ZVM_ROOT=../..
INCLUDE_FLAGS=-I $(ZVM_ROOT)
MACROS_FLAGS=-DUSER_SIDE -DZRT_LIB

all: $(NAME).o
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o $(ZVM_ROOT)/api/libzrt.a
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME).template > $(NAME).manifest
	@$(ZVM_ROOT)/zerovm -e -M$(NAME).manifest -v2

$(NAME).o:
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug *.manifest
