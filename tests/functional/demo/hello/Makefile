NAME=hello

all: $(NAME).o
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME).nexe -s -static \
	-T $(ZVM_TOOLCHAIN)/lib/ldscripts/elf64_nacl.x.static \
	$(NAME).o $(ZEROVM_ROOT)/api/zvm.o
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME).manifest

$(NAME).o: $(NAME).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data debug* *.manifest
