NAME=lz4demo
CCFLAGS=-n -s -nostartfiles -static -nostdlib

all: $(NAME).o lz4.o
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME).nexe $(CCFLAGS) \
	-T $(ZEROVM_ROOT)/tests/functional/include/elf64_nacl.x.static \
	$(NAME).o lz4.o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@$(ZEROVM_ROOT)/zerovm -PQs -M$(NAME).manifest
	@./lz4demo -t output.lz4

$(NAME).o: $(NAME).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

lz4.o: lz4.c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o lz4.o *.log *.lz4 *.manifest
