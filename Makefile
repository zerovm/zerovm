# todo(d'b): replace DEBUG/RELEASE (un)comment with build switch
NACLDEFS=-DNACL_LINUX=1 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_SUBARCH=64 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0
FLAGS0=-fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
CCFLAGS0=-c -m64 -fPIC $(NACLDEFS) -I.
CXXFLAGS0=-m64 -Wno-variadic-macros
TESTFLAGS=-Lgtest -lgtest -lzmq -lrt -ldl -lpthread -lglib-2.0

# RELEASE BUILD
#CCFLAGS=-DNDEBUG -O3 $(CCFLAGS0)
#CXXFLAGS=-DNDEBUG -O3

# DEBUG BUILD
## ABS path is needed to correct gcov, gcov is used to get test coverage
##GCOV_FLAGS=--coverage -g3 -fprofile-arcs -ftest-coverage
##ABS_PATH=$(shell pwd)/
CCFLAGS=-DDEBUG -g $(CCFLAGS0) ${GCOV_FLAGS}
CXXFLAGS=-DDEBUG -g ${GCOV_FLAGS}

CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement $(FLAGS0) $(CCFLAGS)
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare $(CCFLAGS)
CXXFLAGS1=-c -std=c++98 $(CXXFLAGS0) $(FLAGS0) $(NACLDEFS) $(CXXFLAGS) -I.
CXXFLAGS2=-Wl,-z,noexecstack $(CXXFLAGS0) $(CXXFLAGS) -Lobj -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=obj

OBJS=obj/dyn_array.o obj/elf_util.o obj/fault_injection.o obj/gio_mem.o obj/gio_mem_snapshot.o obj/gio.o obj/gprintf.o obj/manifest_parser.o obj/manifest_setup.o obj/md5.o obj/mount_channel.o obj/nacl_all_modules.o obj/nacl_check.o obj/nacl_cpuid.o obj/nacl_dep_qualify_arch.o obj/nacl_dep_qualify.o obj/nacl_desc_base.o obj/nacl_desc_effector_ldr.o obj/nacl_desc_imc_shm.o obj/nacl_desc_io.o obj/nacl_error_code.o obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc_common.o obj/nacl_host_desc.o obj/nacl_imc_c.o obj/nacl_imc.o obj/nacl_imc_unistd.o obj/nacl_log.o obj/nacl_memory_object.o obj/nacl_os_qualify.o obj/nacl_perf_counter.o obj/nacl_refcount.o obj/nacl_signal_64.o obj/nacl_signal_common.o obj/nacl_signal.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/nacl_time.o obj/nacl_xgetbv.o obj/nrd_all_modules.o obj/platform_init.o obj/prefetch.o obj/preload.o obj/sel_addrspace.o obj/sel_addrspace_posix_x86_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_ldr_x86_64.o obj/sel_mem.o obj/sel_memory.o obj/sel_qualify.o obj/sel_rt_64.o obj/sel_segments.o obj/sel_util-inl.o obj/sysv_shm_and_mmap.o obj/tramp_64.o obj/trap.o
#OBJS=$(shell find obj -type f -name '*main*.o' -prune -o -name '*test*.o' -prune -o -name '*.o' -print)

CC=@gcc
CXX=@g++

all: create_dirs zerovm zvm_api tests

create_dirs:
	@mkdir obj test -p

zerovm: obj/sel_main.o $(OBJS)
	$(CXX) -o $@ ${CXXFLAGS2} $^ -lrt -lglib-2.0 -lzmq

gcov: clean all
	@lcov --directory . --base-directory=${ABS_PATH} --capture --output-file app.info
	@genhtml --output-directory cov_htmp app.info
	@echo run ${ABS_PATH}cov_htmp/index.html

tests: test_compile
	@echo == UNIT TESTS ========================================
	@cd test;\
	./manifest_parser_test;\
	./service_runtime_tests;\
#	./manifest_setup_test;\
	cd ..

zvm_api: api/syscall_manager.S api/zrt.c api/zrt.h api/zvm.c api/zvm.h
	@make -Capi

test_compile: test/manifest_parser_test test/manifest_setup_test test/service_runtime_tests

obj/manifest_parser_test.o: src/manifest/manifest_parser_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
test/manifest_parser_test: obj/manifest_parser_test.o $(OBJS)
	$(CXX) ${CXXFLAGS2} -o $@ $^ ${TESTFLAGS}

obj/manifest_setup_test.o: src/manifest/manifest_setup_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
test/manifest_setup_test: obj/manifest_setup_test.o $(OBJS)
	$(CXX) ${CXXFLAGS2} -o $@ $^ ${TESTFLAGS}

obj/sel_ldr_test.o: src/service_runtime/sel_ldr_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/sel_mem_test.o: src/service_runtime/sel_mem_test.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/sel_memory_unittest.o: src/service_runtime/sel_memory_unittest.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
obj/unittest_main.o: src/service_runtime/unittest_main.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
test/service_runtime_tests: obj/sel_ldr_test.o obj/sel_mem_test.o obj/sel_memory_unittest.o obj/unittest_main.o $(OBJS)
	$(CXX) ${CXXFLAGS2} -o $@ $^ ${TESTFLAGS}

.PHONY: clean clean_gcov clean_intermediate clean_api

clean_gcov:
	@find -name "*.gcda" | xargs rm -f
	@find -name "*.gcno" | xargs rm -f
	@rm cov_htmp -f -r

clean: clean_gcov clean_intermediate clean_api
	@rm -f zerovm
	@echo ZeroVM has been deleted

clean_intermediate:
	@rm -f obj/*
	@echo intermediate files has been deleted
	@rm -f test/*
	@echo unit tests has been deleted
	
clean_api:
	@make -Capi clean
	@echo api binaries has been deleted

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

obj/md5.o: src/manifest/md5.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/manifest_parser.o: src/manifest/manifest_parser.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_switch_64.o: src/service_runtime/arch/x86_64/nacl_switch_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/nacl_syscall_64.o: src/service_runtime/arch/x86_64/nacl_syscall_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/tramp_64.o: src/service_runtime/arch/x86_64/tramp_64.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/nacl_cpuid.o: src/service_runtime/nacl_cpuid.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_xgetbv.o: src/service_runtime/nacl_xgetbv.S
	$(CC) ${CCFLAGS2} -o $@ $^

obj/sel_main.o: src/service_runtime/sel_main.c
	$(CC) ${CCFLAGS1} `pkg-config --cflags glib-2.0` -o $@ $^

obj/dyn_array.o: src/service_runtime/dyn_array.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/elf_util.o: src/service_runtime/elf_util.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_all_modules.o: src/service_runtime/nacl_all_modules.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_desc_effector_ldr.o: src/service_runtime/nacl_desc_effector_ldr.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_memory_object.o: src/service_runtime/nacl_memory_object.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal_common.o: src/service_runtime/nacl_signal_common.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_handlers.o: src/service_runtime/nacl_syscall_handlers.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_hook.o: src/service_runtime/nacl_syscall_hook.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_text.o: src/service_runtime/nacl_text.c
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

obj/sel_util-inl.o: src/service_runtime/sel_util-inl.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_switch_to_app_64.o: src/service_runtime/arch/x86_64/nacl_switch_to_app_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_ldr_x86_64.o: src/service_runtime/arch/x86_64/sel_ldr_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_rt_64.o: src/service_runtime/arch/x86_64/sel_rt_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_posix_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_memory.o: src/service_runtime/linux/sel_memory.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sel_segments.o: src/service_runtime/linux/x86/sel_segments.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal.o: src/service_runtime/posix/nacl_signal.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_signal_64.o: src/service_runtime/linux/nacl_signal_64.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_error_code.o: src/service_runtime/nacl_error_code.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_desc_base.o: src/desc/nacl_desc_base.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_desc_imc_shm.o: src/desc/nacl_desc_imc_shm.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_desc_io.o: src/desc/nacl_desc_io.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nrd_all_modules.o: src/desc/nrd_all_modules.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_perf_counter.o: src/perf_counter/nacl_perf_counter.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_refcount.o: src/nacl_base/nacl_refcount.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/fault_injection.o: src/fault_injection/fault_injection.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_os_qualify.o: src/platform_qualify/linux/nacl_os_qualify.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/sysv_shm_and_mmap.o: src/platform_qualify/linux/sysv_shm_and_mmap.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify.o: src/platform_qualify/posix/nacl_dep_qualify.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify_arch.o: src/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_exit.o: src/platform/linux/nacl_exit.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_find_addrsp.o: src/platform/linux/nacl_find_addrsp.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_host_desc.o: src/platform/linux/nacl_host_desc.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_time.o: src/platform/linux/nacl_time.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_check.o: src/platform/nacl_check.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_host_desc_common.o: src/platform/nacl_host_desc_common.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_log.o: src/platform/nacl_log.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/platform_init.o: src/platform/platform_init.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gio.o: src/gio/gio.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gio_mem.o: src/gio/gio_mem.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gprintf.o: src/gio/gprintf.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/gio_mem_snapshot.o: src/gio/gio_mem_snapshot.c
	$(CC) ${CCFLAGS1} -o $@ $^

obj/nacl_imc_c.o: src/imc/nacl_imc_c.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^

obj/nacl_imc_unistd.o: src/imc/nacl_imc_unistd.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^

obj/nacl_imc.o: src/imc/linux/nacl_imc.cc
	$(CXX) ${CXXFLAGS1} -o $@ $^
