NAME=timeout
NACL_TOOLCHAIN=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: $(NAME).o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 $(NAME).o $(ZEROVM_ROOT)/api/libzrt.a
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME).template > $(NAME).manifest
	@binpatch $(NAME).nexe 4c01fc0f3148c1e2 4c01fc909048c1e2
	@setarch x86_64 -R $(ZEROVM_ROOT)/zerovm -e -v1 -M$(NAME).manifest

$(NAME).o: $(NAME).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -I $(ZEROVM_ROOT) \
	-DUSER_SIDE -DZRT_LIB -Wno-long-long -O3 -msse4.1 -m64 -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug *.manifest

