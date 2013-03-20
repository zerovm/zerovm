NAME=zvmlib

all: $(NAME).o bget.o printf.o
	x86_64-nacl-ar r lib$(NAME).a $^

$(NAME).o: $(NAME).c
	x86_64-nacl-gcc -c -Wall -O2 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

bget.o: bget.c
	x86_64-nacl-gcc -c -Wall -O2 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

printf.o: printf.c
	x86_64-nacl-gcc -c -Wall -O2 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f lib$(NAME).a $(NAME).o bget.o printf.o
