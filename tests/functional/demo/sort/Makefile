NAME_1=generator.uint32_t
NAME_2=sort_uint_proper_with_args
NAME_3=tester.uint32_t
NACL_TOOLCHAIN=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: $(NAME_1).o $(NAME_2).o $(NAME_3).o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME_1).nexe -s -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 $(NAME_1).o $(ZEROVM_ROOT)/api/zvm.o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME_2).nexe -s -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 $(NAME_2).o $(ZEROVM_ROOT)/api/zvm.o
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -o $(NAME_3).nexe -s -static -T \
	$(NACL_TOOLCHAIN)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static \
	-melf64_nacl -m64 $(NAME_3).o $(ZEROVM_ROOT)/api/zvm.o
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME_1).template > $(NAME_1).manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME_2).template > $(NAME_2).manifest
	@sed 's#CURDIR#$(CURDIR)#g' $(NAME_3).template > $(NAME_3).manifest
	@ncval_stubout $(NAME_1).nexe -o $(NAME_1).nexe
	@ncval_stubout $(NAME_2).nexe -o $(NAME_2).nexe
	@ncval_stubout $(NAME_3).nexe -o $(NAME_3).nexe
	./run_test.sh

$(NAME_1).o: $(NAME_1).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -DUSER_SIDE -Wno-long-long \
	-O3 -msse4.1 -m64 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

$(NAME_2).o: $(NAME_2).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -DUSER_SIDE -Wno-long-long \
	-O3 -msse4.1 -m64 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

$(NAME_3).o: $(NAME_3).c
	$(NACL_TOOLCHAIN)/bin/x86_64-nacl-gcc -c -Wall -DUSER_SIDE -Wno-long-long \
	-O3 -msse4.1 -m64 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME_1).nexe $(NAME_1).o $(NAME_2).nexe $(NAME_2).o $(NAME_3).nexe $(NAME_3).o *.log *.data *.manifest
