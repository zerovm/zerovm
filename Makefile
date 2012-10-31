NACLDEFS=-DNACL_LINUX=1 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_SUBARCH=64 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0
FLAGS0=-fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
CCFLAGS0=-c -m64 -fPIC $(NACLDEFS) -I.
CXXFLAGS0=-m64 -Wno-variadic-macros
TESTFLAGS=-Llib/gtest -lgtest -lzmq -lrt -ldl -lpthread -lglib-2.0 -lssl -lcrypto

CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement $(FLAGS0) $(CCFLAGS)
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare $(CCFLAGS)
CXXFLAGS1=-c -std=c++98 $(CXXFLAGS0) $(FLAGS0) $(NACLDEFS) $(CXXFLAGS) -I. -Ilib
CXXFLAGS2=-Wl,-z,noexecstack $(CXXFLAGS0) $(CXXFLAGS) -Lobj -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now

OBJS=obj/dyn_array.o obj/elf_util.o obj/gio_mem.o obj/gio_mem_snapshot.o obj/manifest_parser.o obj/manifest_setup.o obj/mount_channel.o obj/nacl_all_modules.o obj/nacl_dep_qualify_arch.o obj/nacl_dep_qualify.o obj/nacl_desc_base.o obj/nacl_exit.o obj/zlog.o obj/nacl_memory_object.o obj/nacl_os_qualify.o obj/nacl_perf_counter.o obj/nacl_refcount.o obj/nacl_signal_64.o obj/nacl_signal_common.o obj/nacl_signal.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_time.o obj/nrd_all_modules.o obj/platform_init.o obj/prefetch.o obj/preload.o obj/sel_addrspace.o obj/sel_addrspace_posix_x86_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_ldr_x86_64.o obj/sel_mem.o obj/sel_memory.o obj/sel_qualify.o obj/sel_rt_64.o obj/sel_segments.o obj/tramp_64.o obj/trap.o obj/etag.o obj/accounting.o

CC=@gcc
CXX=@g++

all: CCFLAGS1 += -DDEBUG -g $(COVFLAGS) $(CCFLAGS0) 
all: CCFLAGS2 += -DDEBUG -g $(COVFLAGS) $(CCFLAGS0) 
all: CXXFLAGS1 := -DDEBUG -g $(COVFLAGS) $(CXXFLAGS1)
all: CXXFLAGS2 := -DDEBUG -g $(COVFLAGS) $(CXXFLAGS2)
all: create_dirs zerovm zvm_api tests

dist: CCFLAGS1 += -DNDEBUG -O3 -s $(CCFLAGS0) 
dist: CCFLAGS2 += -DNDEBUG -O3 -s $(CCFLAGS0) 
dist: CXXFLAGS1 := -DNDEBUG -O3 -s $(CXXFLAGS1)
dist: CXXFLAGS2 := -DNDEBUG -O3 -s $(CXXFLAGS2)
dist: create_dirs zerovm zvm_api tests

create_dirs:
	@mkdir obj -p

zerovm: obj/sel_main.o $(OBJS)
	$(CXX) -o $@ ${CXXFLAGS2} $^ -lrt -lglib-2.0 -lzmq -lssl -lcrypto

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

zvm_api: api/zvm.c api/zvm.h
	@make -Capi

test_compile: tests/unit/manifest_parser_test tests/unit/service_runtime_tests

obj/manifest_parser_test.o: tests/unit/manifest_parser_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
tests/unit/manifest_parser_test: obj/manifest_parser_test.o $(OBJS)
	$(CXX) ${CXXFLAGS2} -o $@ $^ ${TESTFLAGS}

obj/sel_ldr_test.o: tests/unit/sel_ldr_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/sel_mem_test.o: tests/unit/sel_mem_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/sel_memory_unittest.o: tests/unit/sel_memory_unittest.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/unittest_main.o: tests/unit/unittest_main.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
tests/unit/service_runtime_tests: obj/sel_ldr_test.o obj/sel_mem_test.o obj/sel_memory_unittest.o obj/unittest_main.o $(OBJS)
	$(CXX) ${CXXFLAGS2} -o $@ $^ ${TESTFLAGS}

.PHONY: clean clean_gcov clean_intermediate clean_api

clean_gcov:
	@find -name *.gcda -o -name *.gcno | xargs rm -f
	@rm cov_htmp -f -r

clean: clean_gcov clean_intermediate clean_api
	@rm -f zerovm
	@echo ZeroVM has been deleted

clean_intermediate:
	@rm -f tests/unit/manifest_parser_test tests/unit/service_runtime_tests obj/*
	@echo intermediate files has been deleted
	@echo unit tests has been deleted

clean_api:
	@make -Capi clean

obj/mount_channel.o: src/manifest/mount_channel.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/prefetch.o: src/manifest/prefetch.c
	$(CC) ${CCFLAGS1} `pkg-config --cflags glib-2.0` -o $@ $^

obj/preload.o: src/manifest/preload.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/trap.o: src/manifest/trap.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/manifest_setup.o: src/manifest/manifest_setup.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/manifest_parser.o: src/manifest/manifest_parser.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_switch_64.o: src/service_runtime/nacl_switch_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/nacl_syscall_64.o: src/service_runtime/nacl_syscall_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/tramp_64.o: src/service_runtime/tramp_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/sel_main.o: src/service_runtime/sel_main.c
	$(CC) ${CCFLAGS1} `pkg-config --cflags glib-2.0` -o $@ $^

obj/dyn_array.o: src/service_runtime/dyn_array.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/elf_util.o: src/service_runtime/elf_util.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_all_modules.o: src/service_runtime/nacl_all_modules.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_memory_object.o: src/service_runtime/nacl_memory_object.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal_common.o: src/service_runtime/nacl_signal_common.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_handlers.o: src/service_runtime/nacl_syscall_handlers.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_hook.o: src/service_runtime/nacl_syscall_hook.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_addrspace.o: src/service_runtime/sel_addrspace.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_ldr.o: src/service_runtime/sel_ldr.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_ldr_standard.o: src/service_runtime/sel_ldr_standard.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_mem.o: src/service_runtime/sel_mem.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_qualify.o: src/service_runtime/sel_qualify.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_switch_to_app_64.o: src/service_runtime/nacl_switch_to_app_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_x86_64.o: src/service_runtime/sel_addrspace_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_ldr_x86_64.o: src/service_runtime/sel_ldr_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_rt_64.o: src/service_runtime/sel_rt_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_posix_x86_64.o: src/service_runtime/sel_addrspace_posix_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_memory.o: src/service_runtime/sel_memory.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_segments.o: src/service_runtime/sel_segments.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal.o: src/service_runtime/nacl_signal.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal_64.o: src/service_runtime/nacl_signal_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_desc_base.o: src/desc/nacl_desc_base.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nrd_all_modules.o: src/desc/nrd_all_modules.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_perf_counter.o: src/perf_counter/nacl_perf_counter.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_refcount.o: src/nacl_base/nacl_refcount.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_os_qualify.o: src/platform_qualify/nacl_os_qualify.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify.o: src/platform_qualify/nacl_dep_qualify.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify_arch.o: src/platform_qualify/nacl_dep_qualify_arch.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_exit.o: src/platform/nacl_exit.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_time.o: src/platform/nacl_time.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/zlog.o: src/service_runtime/zlog.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/platform_init.o: src/platform/platform_init.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gio_mem.o: src/gio/gio_mem.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gio_mem_snapshot.o: src/gio/gio_mem_snapshot.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/etag.o: src/service_runtime/etag.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/accounting.o: src/service_runtime/accounting.c
	$(CC) ${CCFLAGS1} -o $@ $^
