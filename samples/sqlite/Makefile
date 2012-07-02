NAME=sqlexec_ro
NAME1=main_query
NAME2=vfs_channel
SQLITE=sqlite3
SQLITE_PATH=../../sqlite
NACL_TOOLCHAIN_PATH=~/nacl_sdk/pepper_19/toolchain/linux_x86_glibc
ZVM_ROOT=../..
INCLUDE_FLAGS=-I$(ZVM_ROOT) -I${SQLITE_PATH}
MACROS_FLAGS=-DUSER_SIDE -DZRT_LIB -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION

all: $(NAME1).o $(NAME2).o $(SQLITE).o
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME).nexe -s -static -T \
	$(NACL_TOOLCHAIN_PATH)/x86_64-nacl/lib64/ldscripts/elf64_nacl.x.static -melf64_nacl -m64 \
	$(DEBUG) $(NAME1).o $(NAME2).o $(SQLITE).o $(ZVM_ROOT)/api/libzrt.a
	#@$(ZVM_ROOT)/zerovm -M$(NAME).manifest -v2

$(SQLITE).o:
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(SQLITE).o -Wall \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${SQLITE_PATH}/${SQLITE}.c

$(NAME1).o: $(NAME1).c
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME1).o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${NAME1}.c

$(NAME2).o: $(NAME2).c
	$(NACL_TOOLCHAIN_PATH)/bin/x86_64-nacl-gcc -o $(NAME2).o -Wall -std=c99 \
	$(INCLUDE_FLAGS) $(MACROS_FLAGS) $(DEBUG) -c -Wno-long-long -O2 -m64 ${NAME2}.c


clean:
	rm -f $(NAME).nexe *.o *.log *.result
