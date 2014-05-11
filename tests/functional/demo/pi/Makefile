NAME=pi
CCFLAGS=-n -s -nostartfiles -nostdlib -fno-builtin -D__ZEROVM__

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $^ \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@echo pi\\n1\\n\;\\nenv=0\\nenv=1\\nenv=last > nvram
	@$(ZEROVM_ROOT)/zerovm $(NAME).manifest

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest nvram
