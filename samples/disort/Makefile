NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_16/toolchain/linux_x86
MACROS_FLAGS=-DUSER_SIDE -DZRT_LIB
INCLUDE_FLAGS=-I ~/git/zerovm -I.
ZRT_OBJ=~/git/zerovm/api/zrt.o ~/git/zerovm/api/zvm.o ~/git/zerovm/api/syscall_manager.o
DEBUG=-g
MANNODE=nodeman
SRCNODE=nodesrc
DSTNODE=nodedst

all: prepare $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe
	
prepare: createdirs
	@chmod u=rwx gendata.sh
	@chmod u=rwx genmanifest.sh
	@./gendata.sh
	@./genmanifest.sh
 	
createdirs:
	@mkdir -p obj log data vmlog

$(MANNODE).nexe: obj/main_man.o obj/comm_man.o obj/histanlz.o obj/sort.o obj/dsort.o 
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(MANNODE).nexe  -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_man.o obj/comm_man.o obj/histanlz.o obj/sort.o obj/dsort.o -Wall -std=c99 \
	$(ZRT_OBJ) $(MANNODE_INCL) $(DEBUG)  
	
$(SRCNODE).nexe: obj/main_src.o obj/sort.o obj/dsort.o obj/comm_src.o 
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(SRCNODE).nexe  -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_src.o obj/sort.o obj/dsort.o obj/comm_src.o -Wall -std=c99 \
	$(ZRT_OBJ) $(SRCNODE_INCL) $(DEBUG)  
	
$(DSTNODE).nexe: obj/main_dst.o obj/sort.o obj/dsort.o obj/comm_dst.o 
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(DSTNODE).nexe  -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	obj/main_dst.o obj/sort.o obj/dsort.o obj/comm_dst.o -Wall -std=c99 \
	$(ZRT_OBJ) $(DSTNODE_INCL) $(DEBUG)  

#source node & dest node
obj/dsort.o: dsort.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/dsort.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 dsort.c

#source node & dest node
obj/sort.o: sort.c
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/sort.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 sort.c

#destination node
obj/main_dst.o: dest/main_dst.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/main_dst.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Idest dest/main_dst.c

#destination node
obj/comm_dst.o: dest/comm_dst.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/comm_dst.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Idest dest/comm_dst.c

#source node
obj/main_src.o: source/main_src.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/main_src.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource source/main_src.c
	
#source node
obj/comm_src.o: source/comm_src.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/comm_src.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource source/comm_src.c

#manager node
obj/main_man.o: manager/main_man.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/main_man.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Isource manager/main_man.c

#manager node
obj/comm_man.o: manager/comm_man.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/comm_man.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Imanager manager/comm_man.c

#manager node
obj/histanlz.o: manager/histanlz.c defines.h
	@$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -c -o obj/histanlz.o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -Wno-long-long -O2 -msse4.1 -m64 -Imanager manager/histanlz.c

clean: 
	@rm -f *.nexe
	@rm -f obj/*.o
	@rm -f vmlog/*
	@rm -f data/*.data
	@rm -f $(MANNODE).nexe $(SRCNODE).nexe $(DSTNODE).nexe
	

