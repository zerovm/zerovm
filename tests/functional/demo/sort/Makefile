NAME_1=generator.uint32_t
NAME_2=sort_uint_proper_with_args
NAME_3=tester.uint32_t
CCFLAGS=-n -s -nostartfiles -nostdlib 

all: $(NAME_1).c $(NAME_2).c $(NAME_3).c
	@x86_64-nacl-gcc -o $(NAME_1).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $(NAME_1).c \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@x86_64-nacl-gcc -o $(NAME_2).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O3 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $(NAME_2).c \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@x86_64-nacl-gcc -o $(NAME_3).nexe $(CCFLAGS) -Wall -msse4.1 \
	-O2 -I$(ZEROVM_ROOT) -I$(ZEROVM_ROOT)/tests/functional $(NAME_3).c \
	$(ZEROVM_ROOT)/tests/functional/include/libzvmlib.a
	@sed 's#PWD#$(PWD)#g' $(NAME_1).template > $(NAME_1).manifest
	@sed 's#PWD#$(PWD)#g' $(NAME_2).template > $(NAME_2).manifest
	@sed 's#PWD#$(PWD)#g' $(NAME_3).template > $(NAME_3).manifest
	./run_test.sh

clean:
	rm -f $(NAME_1).nexe $(NAME_1).o $(NAME_2).nexe $(NAME_2).o \
	$(NAME_3).nexe $(NAME_3).o *.log *.data *.manifest
