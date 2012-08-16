PNACL_TOOL=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
DEBUG=-DDEBUG
ZRT_ROOT=../..
INCLUDE_FLAGS=-I$(ZRT_ROOT)/lib -I$(ZRT_ROOT)/zvm

all: createdirs reqrep.nexe run
	
reqrep.nexe: obj/reqrep.o
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -o reqrep.nexe  -s -static -T \
	$(PNACL_TOOL)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/reqrep.o -Wall -std=c99 -L${ZRT_ROOT}/lib -lzrt $(DEBUG)  
	
obj/reqrep.o: reqrep.c
	@$(PNACL_TOOL)/bin/x86_64-nacl-gcc -c -o obj/reqrep.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(DEBUG) -Wno-long-long -m64 reqrep.c
	
run:
	./genmanifest.sh
	./test.sh	

createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f *.nexe
	@rm -f *.manifest
	

