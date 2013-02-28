NAME_1=generator.uint32_t
NAME_2=sort_uint_proper_with_args
NAME_3=tester.uint32_t
NACL_TOOLCHAIN=$(NACL_SDK_ROOT)/toolchain/linux_x86_glibc

all: $(NAME_1).o $(NAME_2).o $(NAME_3).o
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME_1).nexe -s -static \
	-T $(ZVM_TOOLCHAIN)/lib/ldscripts/elf64_nacl.x.static \
	$(NAME_1).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME_2).nexe -s -static \
	-T $(ZVM_TOOLCHAIN)/lib/ldscripts/elf64_nacl.x.static \
	$(NAME_2).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME_3).nexe -s -static \
	-T $(ZVM_TOOLCHAIN)/lib/ldscripts/elf64_nacl.x.static \
	$(NAME_3).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME_1).template > $(NAME_1).manifest
	@sed 's#PWD#$(PWD)#g' $(NAME_2).template > $(NAME_2).manifest
	@sed 's#PWD#$(PWD)#g' $(NAME_3).template > $(NAME_3).manifest
	./run_test.sh

$(NAME_1).o: $(NAME_1).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

$(NAME_2).o: $(NAME_2).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

$(NAME_3).o: $(NAME_3).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME_1).nexe $(NAME_1).o $(NAME_2).nexe $(NAME_2).o $(NAME_3).nexe $(NAME_3).o *.log *.data *.manifest
