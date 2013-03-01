NAME=netcopy

all: $(NAME).o
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME).nexe -n -s -nostartfiles -static \
	-T $(ZEROVM_ROOT)/tests/functional/include/elf64_nacl.x.static \
	$(NAME).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@python $(CURDIR)/ns_server.py 3 54321&
	@sed 's#PWD#$(PWD)#g' $(NAME)1.template > $(NAME)1.manifest
	@sed 's#PWD#$(PWD)#g' $(NAME)2.template > $(NAME)2.manifest
	@sed 's#PWD#$(PWD)#g' $(NAME)3.template > $(NAME)3.manifest
	@sleep 1
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)1.manifest&
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)2.manifest&
	@$(ZEROVM_ROOT)/zerovm -e -M$(NAME)3.manifest

$(NAME).o: $(NAME).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
