NAME=netcopy
NACL_TOOLCHAIN=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: $(NAME).o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 $(NAME).o $(ZEROVM_ROOT)/api/zvm.o
	@python $(CURDIR)/ns_server.py 3 54321&
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)1.template > $(NAME)1.manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)2.template > $(NAME)2.manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME)3.template > $(NAME)3.manifest
	@sleep 1
	@ncval_stubout $(NAME).nexe -o $(NAME).nexe
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)1.manifest&
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)2.manifest&
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)3.manifest

$(NAME).o: $(NAME).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -DUSER_SIDE -Wno-long-long \
	-O3 -msse4.1 -m64 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
