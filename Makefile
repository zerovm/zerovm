define PREFIX_ERR
Please set up ZVM_PREFIX env variable to the desired installation path
Example: export ZVM_PREFIX=/opt/zerovm
endef

ifndef ZVM_PREFIX
$(error $(PREFIX_ERR))
endif

FLAGS0=-fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
GLIB=`pkg-config --cflags glib-2.0`
CCFLAGS0=-c -m64 -fPIC -D_GNU_SOURCE=1 -I. $(GLIB)
CXXFLAGS0=-m64 -Wno-variadic-macros $(GLIB)
LIBS=-L$(ZVM_PREFIX) -lzmq -lglib-2.0 -lvalidator -Wl,-rpath,$(ZVM_PREFIX)
TESTLIBS=-Llib/gtest -lgtest -Wl,-rpath,$(ZVM_PREFIX) $(LIBS)

CCFLAGS1=-std=gnu89 -Wdeclaration-after-statement $(FLAGS0) $(CCFLAGS0)
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare $(CCFLAGS0)
CXXFLAGS1=-c -std=c++98 -D_GNU_SOURCE=1 -I. -Ilib $(CXXFLAGS0) $(FLAGS0)
CXXFLAGS2=-Wl,-z,noexecstack $(CXXFLAGS0) -Lobj -pie -Wl,-z,relro -Wl,-z,now

all: CCFLAGS1 += -DNDEBUG -O2 -s
all: CCFLAGS2 += -DNDEBUG -O2 -s
all: CXXFLAGS1 := -DNDEBUG -O2 -s $(CXXFLAGS1)
all: CXXFLAGS2 := -DNDEBUG -O2 -s $(CXXFLAGS2)
all: create_dirs zerovm

debug: CCFLAGS1 += -DDEBUG -g
debug: CCFLAGS2 += -DDEBUG -g
debug: CXXFLAGS1 := -DDEBUG -g $(CXXFLAGS1)
debug: CXXFLAGS2 := -DDEBUG -g $(CXXFLAGS2)
debug: create_dirs zerovm tests

OBJS=obj/elf_util.o obj/gio_mem.o obj/gio_mem_snapshot.o obj/manifest_parser.o obj/manifest_setup.o obj/mount_channel.o obj/nacl_dep_qualify.o obj/nacl_exit.o obj/zlog.o obj/nacl_signal_64.o obj/nacl_signal_common.o obj/nacl_signal.o obj/side_switch.o obj/switch_to_app.o obj/trap_syscall.o obj/syscall_hook.o obj/prefetch.o obj/name_service.o obj/preload.o obj/sel_addrspace.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_ldr_x86_64.o obj/sel_memory.o obj/sel_qualify.o obj/sel_rt.o obj/tramp.o obj/trap.o obj/etag.o obj/accounting.o
CC=@gcc
CXX=@g++

create_dirs:
	@mkdir obj -p

zerovm: obj/zvm_main.o $(OBJS)
	$(CC) -o $@ $(CXXFLAGS2) $^ $(LIBS)

tests: test_compile
	@printf "UNIT TESTS %048o\n" 0
	@cd tests/unit;\
	./manifest_parser_test;\
	./service_runtime_tests;\
	cd ..

test_compile: tests/unit/manifest_parser_test tests/unit/service_runtime_tests

obj/manifest_parser_test.o: tests/unit/manifest_parser_test.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
tests/unit/manifest_parser_test: obj/manifest_parser_test.o $(OBJS)
	$(CXX) $(CXXFLAGS2) -o $@ $^ $(TESTLIBS)

obj/sel_ldr_test.o: tests/unit/sel_ldr_test.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
obj/sel_memory_unittest.o: tests/unit/sel_memory_unittest.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
obj/unittest_main.o: tests/unit/unittest_main.cc
	$(CXX) $(CXXFLAGS1) -o $@ $^
tests/unit/service_runtime_tests: obj/sel_ldr_test.o obj/sel_memory_unittest.o obj/unittest_main.o $(OBJS)
	$(CXX) $(CXXFLAGS2) -o $@ $^ $(TESTLIBS)

.PHONY: clean clean_intermediate install

clean: clean_intermediate
	@rm -f zerovm
	@echo ZeroVM has been deleted

clean_intermediate:
	@rm -f tests/unit/manifest_parser_test tests/unit/service_runtime_tests obj/*
	@echo intermediate files has been deleted
	@echo unit tests has been deleted

install:
	install -D -m 0755 zerovm $(ZVM_PREFIX)/zerovm
	install -D -m 0644 api/zvm.h $(ZVM_PREFIX)/api/zvm.h

obj/mount_channel.o: src/channels/mount_channel.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/prefetch.o: src/channels/prefetch.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/name_service.o: src/channels/name_service.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/preload.o: src/channels/preload.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/trap.o: src/syscalls/trap.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/manifest_setup.o: src/main/manifest_setup.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/manifest_parser.o: src/main/manifest_parser.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/side_switch.o: src/syscalls/side_switch.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/trap_syscall.o: src/syscalls/trap_syscall.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/tramp.o: src/syscalls/tramp.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/zvm_main.o: src/main/zvm_main.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/elf_util.o: src/loader/elf_util.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal_common.o: src/platform/nacl_signal_common.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/syscall_hook.o: src/syscalls/syscall_hook.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_addrspace.o: src/loader/sel_addrspace.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr.o: src/loader/sel_ldr.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr_standard.o: src/loader/sel_ldr_standard.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_qualify.o: src/platform/sel_qualify.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/switch_to_app.o: src/syscalls/switch_to_app.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr_x86_64.o: src/loader/sel_ldr_x86_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_rt.o: src/loader/sel_rt.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_memory.o: src/platform/sel_memory.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal.o: src/platform/nacl_signal.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal_64.o: src/platform/nacl_signal_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_dep_qualify.o: src/platform/nacl_dep_qualify.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_exit.o: src/main/nacl_exit.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/zlog.o: src/main/zlog.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/gio_mem.o: src/platform/gio_mem.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/gio_mem_snapshot.o: src/platform/gio_mem_snapshot.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/etag.o: src/main/etag.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/accounting.o: src/main/accounting.c
	$(CC) $(CCFLAGS1) -o $@ $^
