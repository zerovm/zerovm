FLAGS0=-fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
GLIB=`pkg-config --cflags glib-2.0`
CCFLAGS0=-c -m64 -fPIC -D_GNU_SOURCE=1 -I. $(GLIB)
CXXFLAGS0=-m64 -Wno-variadic-macros $(GLIB)
LIBS=-lzmq -lglib-2.0
TESTLIBS=-Llib/gtest -lgtest $(LIBS)

CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement $(FLAGS0) $(CCFLAGS0)
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare $(CCFLAGS0)
CXXFLAGS1=-c -std=c++98 -D_GNU_SOURCE=1 -I. -Ilib $(CXXFLAGS0) $(FLAGS0)
CXXFLAGS2=-Wl,-z,noexecstack $(CXXFLAGS0) -Lobj -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now

# to compile zvm w/o nacl syscalls support -DDISABLE_NACL_SYSCALLS should be specidied
all: CCFLAGS1 += -DNDEBUG -O3 -s -DDISABLE_NACL_SYSCALLS
all: CCFLAGS2 += -DNDEBUG -O3 -s -DDISABLE_NACL_SYSCALLS
all: CXXFLAGS1 := -DNDEBUG -O3 -s $(CXXFLAGS1) -DDISABLE_NACL_SYSCALLS
all: CXXFLAGS2 := -DNDEBUG -O3 -s $(CXXFLAGS2) -DDISABLE_NACL_SYSCALLS
all: create_dirs zerovm

debug: CCFLAGS1 += -DDEBUG -g -DDISABLE_NACL_SYSCALLS
debug: CCFLAGS2 += -DDEBUG -g -DDISABLE_NACL_SYSCALLS
debug: CXXFLAGS1 := -DDEBUG -g $(CXXFLAGS1) -DDISABLE_NACL_SYSCALLS
debug: CXXFLAGS2 := -DDEBUG -g $(CXXFLAGS2) -DDISABLE_NACL_SYSCALLS

# d'b
#debug: create_dirs zerovm zvm_api tests
debug: create_dirs zerovm tests

OBJS=obj/elf_util.o obj/gio_mem.o obj/gio_mem_snapshot.o obj/manifest_parser.o obj/manifest_setup.o obj/mount_channel.o obj/nacl_dep_qualify.o obj/nacl_exit.o obj/zlog.o obj/nacl_os_qualify.o obj/nacl_signal_64.o obj/nacl_signal_common.o obj/nacl_signal.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/prefetch.o obj/name_service.o obj/preload.o obj/sel_addrspace.o obj/sel_addrspace_posix_x86_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_ldr_x86_64.o obj/sel_memory.o obj/sel_qualify.o obj/sel_rt_64.o obj/sel_segments.o obj/tramp_64.o obj/trap.o obj/etag.o obj/accounting.o
CC=@gcc
CXX=@g++

create_dirs:
	@mkdir obj -p

zerovm: obj/zvm_main.o $(OBJS)
	$(CXX) -o $@ $(CXXFLAGS2) $^ $(LIBS)

gcov: clean all
	@lcov --directory . --base-directory=$(ZEROVM_ROOT) --capture --output-file app.info
	@genhtml --output-directory cov_htmp app.info
	@echo open $(ZEROVM_ROOT)/cov_htmp/index.html

tests: test_compile
	@echo == UNIT TESTS ========================================
	@cd tests/unit;\
	./manifest_parser_test;\
	./service_runtime_tests;\
	cd ..

# d'b
#zvm_api: api/zvm.c api/zvm.h
#	@make -Capi

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

# d'b
#.PHONY: clean clean_gcov clean_intermediate clean_api
.PHONY: clean clean_gcov clean_intermediate

clean_gcov:
	@find -name *.gcda -o -name *.gcno | xargs rm -f
	@rm cov_htmp -f -r

# d'b
#clean: clean_gcov clean_intermediate clean_api
clean: clean_gcov clean_intermediate
	@rm -f zerovm
	@echo ZeroVM has been deleted

clean_intermediate:
	@rm -f tests/unit/manifest_parser_test tests/unit/service_runtime_tests obj/*
	@echo intermediate files has been deleted
	@echo unit tests has been deleted

# d'b
#clean_api:
#	@make -Capi clean

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

obj/nacl_switch_64.o: src/syscalls/nacl_switch_64.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/nacl_syscall_64.o: src/syscalls/nacl_syscall_64.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/tramp_64.o: src/syscalls/tramp_64.S
	$(CC) $(CCFLAGS2) -o $@ $^

obj/zvm_main.o: src/main/zvm_main.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/elf_util.o: src/loader/elf_util.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal_common.o: src/platform/nacl_signal_common.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_syscall_handlers.o: src/syscalls/nacl_syscall_handlers.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_syscall_hook.o: src/syscalls/nacl_syscall_hook.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_addrspace.o: src/loader/sel_addrspace.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr.o: src/loader/sel_ldr.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr_standard.o: src/loader/sel_ldr_standard.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_qualify.o: src/platform/sel_qualify.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_switch_to_app_64.o: src/syscalls/nacl_switch_to_app_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_addrspace_x86_64.o: src/loader/sel_addrspace_x86_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_ldr_x86_64.o: src/loader/sel_ldr_x86_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_rt_64.o: src/loader/sel_rt_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_addrspace_posix_x86_64.o: src/loader/sel_addrspace_posix_x86_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_memory.o: src/platform/sel_memory.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/sel_segments.o: src/loader/sel_segments.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal.o: src/platform/nacl_signal.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_signal_64.o: src/platform/nacl_signal_64.c
	$(CC) $(CCFLAGS1) -o $@ $^

obj/nacl_os_qualify.o: src/platform/nacl_os_qualify.c
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
