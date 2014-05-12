NAME=ztrace
CCFLAGS=-n -s -nostartfiles -nostdlib -fno-builtin -D__ZEROVM__

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $^ \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@strip -R .comment $(NAME).nexe
	@strip -R .eh_frame $(NAME).nexe
	@touch $(NAME).trc
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@$(ZEROVM_ROOT)/zerovm $(NAME).manifest -T$(NAME).trc

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest *.trc
