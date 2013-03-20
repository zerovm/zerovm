NAME=api
CCFLAGS=-n -s -nostartfiles -static -nostdlib

all: $(NAME).o
	$(ZVM_TOOLCHAIN)/bin/gcc -o $(NAME).nexe $(CCFLAGS) \
	$(NAME).o $(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@$(ZEROVM_ROOT)/zerovm -e3 $(NAME).manifest

$(NAME).o: $(NAME).c
	touch glib.h
	$(ZVM_TOOLCHAIN)/bin/gcc -c -Wall -O3 -msse4.1 -I`pwd` \
	-I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional -o $@ $^

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest glib.h
