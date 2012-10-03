NAME=api_test
NACL_TOOLCHAIN_PATH=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc
ZVM_ROOT=../..
INCLUDE_FLAGS=-I $(ZVM_ROOT)
MACROS_FLAGS=-DUSER_SIDE

all: $(NAME).o
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME).o $(ZVM_ROOT)/api/zvm.o
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME).template > $(NAME).manifest
	@binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	@setarch x86_64 -R $(ZVM_ROOT)/zerovm -e -M$(NAME).manifest

$(NAME).o:
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -msse4.1 -m64 $(NAME).c

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.dat debug *.manifest
