NAME=fork
CCFLAGS=-n -s -nostartfiles -nostdlib -fno-builtin

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $^ \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@sed 's#PWD#$(PWD)#g' $(NAME)ed.template > $(NAME)ed.manifest
	@$(ZEROVM_ROOT)/zerovm $(NAME).manifest

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest fork_test LOG
	pkill -9 zvm. | true
