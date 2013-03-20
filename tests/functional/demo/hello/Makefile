NAME=hello
CCFLAGS=-n -s -nostartfiles -nostdlib 

all: $(NAME).o
	x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) \
	$(NAME).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@$(ZEROVM_ROOT)/zerovm -e3 $(NAME).manifest

$(NAME).o: $(NAME).c
	x86_64-nacl-gcc -c -Wall -O3 -msse4.1 \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
