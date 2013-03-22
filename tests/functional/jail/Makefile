NAME=jail
CCFLAGS=-n -s -nostartfiles -nostdlib 

all: $(NAME).c
	@x86_64-nacl-gcc -o $(NAME).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $^ \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME).template > $(NAME).manifest
	@zerovm -e3 $(NAME).manifest

clean:
	rm -f $(NAME).nexe $(NAME).o *.log *.data *.manifest
