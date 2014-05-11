NAME=boot
CCFLAGS=-n -s -nostartfiles -nostdlib

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) $^
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@objcopy -O binary --only-section=.text $(NAME).nexe $(NAME).bin
	@xxd -i $(NAME).bin > $(NAME).inc
	@$(ZEROVM_ROOT)/zerovm $(NAME).manifest

clean:
	rm -f $(NAME).nexe $(NAME).o *.manifest *.bin *.inc
