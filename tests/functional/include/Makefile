NAME=zvmlib

all: $(NAME).o bget.o printf.o
	$(ZVM_TOOLCHAIN)/bin/ar r lib$(NAME).a $^

$(NAME).o: $(NAME).c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

bget.o: bget.c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

printf.o: printf.c
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 $(DEFS) \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f lib$(NAME).a $(NAME).o bget.o printf.o
