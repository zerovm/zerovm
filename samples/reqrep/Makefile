PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
NAME=malloc
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm

all: createdirs clean test1.nexe test2.nexe
	
test1.nexe: obj/main_test1.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o test1.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_test1.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt $(DEBUG)  
	
test2.nexe: obj/main_test2.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o test2.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_test2.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt $(DEBUG)  	

obj/main_test1.o: main_test1.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/main_test1.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -m64 main_test1.c

obj/main_test2.o: main_test2.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/main_test2.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -m64 main_test2.c
	
createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f *.nexe
	

