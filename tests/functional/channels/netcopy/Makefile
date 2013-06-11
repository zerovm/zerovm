NAME=netcopy
CCFLAGS=-n -s -nostartfiles -nostdlib -fno-builtin

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $^ \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@python $(CURDIR)/ns_server.py 3 54321&
	@sed 's#PWD#$(PWD)#g' $(NAME)1.template > $(NAME)1.manifest
	@sed 's#PWD#$(PWD)#g' $(NAME)2.template > $(NAME)2.manifest
	@sed 's#PWD#$(PWD)#g' $(NAME)3.template > $(NAME)3.manifest
	@echo copier1 > nvram1
	@echo copier2 > nvram2
	@echo copier3 > nvram3
	@dd if=/dev/urandom of=input.data bs=1048576 count=32 2> /dev/null
	@$(ZEROVM_ROOT)/zerovm -e3 $(NAME)1.manifest&
	@$(ZEROVM_ROOT)/zerovm -e3 $(NAME)2.manifest&
	@$(ZEROVM_ROOT)/zerovm -e3 $(NAME)3.manifest
	@pkill -f ns_server

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest nvram*

