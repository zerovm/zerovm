NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain/linux_x86
MACROS_FLAGS=-DUSER_SIDE -DZRT_LIB
INCLUDE_FLAGS=-I ~/git/zerovm -I.
ZRT_OBJ=~/git/zerovm/api/zrt.o ~/git/zerovm/api/zvm.o ~/git/zerovm/api/syscall_manager.o
DEBUG=-g

all: createdirs clean test1.nexe test2.nexe
	
test1.nexe: obj/main_test1.o
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o test1.nexe  -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_test1.o -Wall -std=c99 \
	$(ZRT_OBJ) $(MANNODE_INCL) $(DEBUG)  
	
test2.nexe: obj/main_test2.o
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o test2.nexe  -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_test2.o -Wall -std=c99 \
	$(ZRT_OBJ) $(MANNODE_INCL) $(DEBUG)  	

obj/main_test1.o: main_test1.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/main_test1.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -m64 main_test1.c

obj/main_test2.o: main_test2.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/main_test2.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -m64 main_test2.c
	
createdirs:
	@mkdir -p obj log

clean: 
	@rm -f obj/*.o
	@rm -f log/*.log
	@rm -f *.nexe
	

