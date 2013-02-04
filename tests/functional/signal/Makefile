# this makefile has option to build nexe with debug information
# so it is possible to view an extra information about the signal
# with the command:
# addr2line -e signal.nexe 0x20d92 -f
# pay attention to the address, it is the same value as returned
# by zerovm but corrected to local adress space. to do so you
# need substract 0x440A00000000 from the real address

NAME=signal
NACL_TOOLCHAIN=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: $(NAME).o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME).nexe -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 -g $(NAME).o $(ZEROVM_ROOT)/api/zvm.o
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME).template > $(NAME).manifest 
	@ncval_stubout $(NAME).nexe -o $(NAME).nexe
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME).manifest

$(NAME).o: $(NAME).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -DUSER_SIDE -Wno-long-long \
	-O3 -msse4.1 -m64 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug* *.manifest
