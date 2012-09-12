# todo(d'b): replace DEBUG/RELEASE (un)comment with build switch
# RELEASE BUILD
#CCFLAGS=-DNDEBUG -O3
#CXXFLAGS=-DNDEBUG -O3

# DEBUG BUILD
## ABS path is needed to correct gcov, gcov is used to get test coverage
##GCOV_FLAGS=--coverage -g3 -fprofile-arcs -ftest-coverage
##ABS_PATH=$(shell pwd)/
CCFLAGS=-DDEBUG -g ${GCOV_FLAGS}
CXXFLAGS=-DDEBUG -g ${GCOV_FLAGS}

CCFLAGS0=-c -m64 -fPIC -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare
CCFLAGS3=-fno-strict-aliasing -Wno-missing-field-initializers
CCFLAGS4=-DNACL_TRUSTED_BUT_NOT_TCB
CCFLAGS6=-DUNIT_TEST
CXXFLAGS1=-c -std=c++98 -Wno-variadic-macros -m64 -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4 -DNACL_TRUSTED_BUT_NOT_TCB -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CXXFLAGS2=-Wl,-z,noexecstack -m64 -Wno-variadic-macros -Lobj -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=obj
CXXFLAGS3=-c -std=c++98 -Wno-variadic-macros -m64 -fPIE -Wall -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4 -DNACL_TRUSTED_BUT_NOT_TCB -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.

OBJS=obj/dyn_array.o obj/elf_util.o obj/fault_injection.o obj/gio_mem.o obj/gio_mem_snapshot.o obj/gio.o obj/gprintf.o obj/manifest_parser.o obj/manifest_setup.o obj/md5.o obj/mount_channel.o obj/nacl_all_modules.o obj/nacl_check.o obj/nacl_cpuid.o obj/nacl_dep_qualify_arch.o obj/nacl_dep_qualify.o obj/nacl_desc_base.o obj/nacl_desc_effector_ldr.o obj/nacl_desc_imc_shm.o obj/nacl_desc_io.o obj/nacl_error_code.o obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc_common.o obj/nacl_host_desc.o obj/nacl_imc_c.o obj/nacl_imc.o obj/nacl_imc_unistd.o obj/nacl_log.o obj/nacl_memory_object.o obj/nacl_os_qualify.o obj/nacl_perf_counter.o obj/nacl_refcount.o obj/nacl_signal_64.o obj/nacl_signal_common.o obj/nacl_signal.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/nacl_time.o obj/nacl_xgetbv.o obj/nrd_all_modules.o obj/platform_init.o obj/prefetch.o obj/preload.o obj/sel_addrspace.o obj/sel_addrspace_posix_x86_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_ldr_x86_64.o obj/sel_mem.o obj/sel_memory.o obj/sel_qualify.o obj/sel_rt_64.o obj/sel_segments.o obj/sel_util-inl.o obj/sysv_shm_and_mmap.o obj/tramp_64.o obj/trap.o

all: create_dirs zerovm zvm_api tests 

create_dirs: 
	@mkdir obj -p
	@mkdir test -p

zerovm: obj/sel_main.o ${OBJS}
	@g++ -o $@ ${CXXFLAGS} ${CXXFLAGS2} $^ -lrt -lglib-2.0 -lzmq

gcov: clean all
	@lcov --directory . --base-directory=${ABS_PATH} --capture --output-file app.info
	@genhtml --output-directory cov_htmp app.info
	@echo run ${ABS_PATH}cov_htmp/index.html

tests: test_compile 
	@echo == unit tests ================================================
	@cd test;\
	./manifest_parser_test;\
#	./manifest_setup_test;\
	./service_runtime_tests;\
	cd ..

zvm_api: api/syscall_manager.S api/zrt.c api/zrt.h api/zvm.c api/zvm.h
	@make -Capi

test_compile: test/manifest_parser_test test/manifest_setup_test test/service_runtime_tests

obj/manifest_parser_test.o: src/manifest/manifest_parser_test.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} -o $@ $^
test/manifest_parser_test: obj/manifest_parser_test.o ${OBJS}
	@g++ ${CXXFLAGS} ${CXXFLAGS2} -o $@ $^ -Lgtest -lgtest -lzmq -lrt -ldl -lpthread -lglib-2.0

obj/manifest_setup_test.o: src/manifest/manifest_setup_test.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} -o $@ $^
test/manifest_setup_test: obj/manifest_setup_test.o ${OBJS}
	@g++ ${CXXFLAGS} ${CXXFLAGS2} -o $@ $^ -Lgtest -lgtest -lzmq -lrt -ldl -lpthread -lglib-2.0

obj/sel_ldr_test.o: src/service_runtime/sel_ldr_test.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^
obj/sel_mem_test.o: src/service_runtime/sel_mem_test.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^
obj/sel_memory_unittest.o: src/service_runtime/sel_memory_unittest.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^
obj/unittest_main.o: src/service_runtime/unittest_main.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^
test/service_runtime_tests: obj/sel_ldr_test.o obj/sel_mem_test.o obj/sel_memory_unittest.o obj/unittest_main.o ${OBJS}
	@g++ ${CXXFLAGS} ${CXXFLAGS2} -o $@ $^ -Lgtest -lgtest -lzmq -lrt -ldl -lpthread -lglib-2.0

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
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} -o $@ $^

obj/prefetch.o: src/manifest/prefetch.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} `pkg-config --cflags glib-2.0` -o $@ $^

obj/preload.o: src/manifest/preload.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} -o $@ $^

obj/trap.o: src/manifest/trap.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} -o $@ $^

obj/manifest_setup.o: src/manifest/manifest_setup.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} -o $@ $^

obj/md5.o: src/manifest/md5.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} -o $@ $^

obj/manifest_parser.o: src/manifest/manifest_parser.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_switch_64.o: src/service_runtime/arch/x86_64/nacl_switch_64.S
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS2} -o $@ $^

obj/nacl_syscall_64.o: src/service_runtime/arch/x86_64/nacl_syscall_64.S
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS2} -o $@ $^

obj/tramp_64.o: src/service_runtime/arch/x86_64/tramp_64.S
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS2} -o $@ $^

obj/nacl_cpuid.o: src/service_runtime/nacl_cpuid.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_xgetbv.o: src/service_runtime/nacl_xgetbv.S
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS2} -o $@ $^

obj/sel_main.o: src/service_runtime/sel_main.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS5} `pkg-config --cflags glib-2.0` -o $@ $^

obj/dyn_array.o: src/service_runtime/dyn_array.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/elf_util.o: src/service_runtime/elf_util.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_all_modules.o: src/service_runtime/nacl_all_modules.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_desc_effector_ldr.o: src/service_runtime/nacl_desc_effector_ldr.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_memory_object.o: src/service_runtime/nacl_memory_object.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_signal_common.o: src/service_runtime/nacl_signal_common.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_handlers.o: src/service_runtime/nacl_syscall_handlers.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_syscall_hook.o: src/service_runtime/nacl_syscall_hook.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_text.o: src/service_runtime/nacl_text.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_addrspace.o: src/service_runtime/sel_addrspace.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_ldr.o: src/service_runtime/sel_ldr.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_ldr_standard.o: src/service_runtime/sel_ldr_standard.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_mem.o: src/service_runtime/sel_mem.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_qualify.o: src/service_runtime/sel_qualify.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_util-inl.o: src/service_runtime/sel_util-inl.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_switch_to_app_64.o: src/service_runtime/arch/x86_64/nacl_switch_to_app_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_x86_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_ldr_x86_64.o: src/service_runtime/arch/x86_64/sel_ldr_x86_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_rt_64.o: src/service_runtime/arch/x86_64/sel_rt_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_addrspace_posix_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_memory.o: src/service_runtime/linux/sel_memory.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sel_segments.o: src/service_runtime/linux/x86/sel_segments.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_signal.o: src/service_runtime/posix/nacl_signal.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_signal_64.o: src/service_runtime/linux/nacl_signal_64.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_error_code.o: src/service_runtime/nacl_error_code.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_desc_base.o: src/desc/nacl_desc_base.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_desc_imc_shm.o: src/desc/nacl_desc_imc_shm.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_desc_io.o: src/desc/nacl_desc_io.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nrd_all_modules.o: src/desc/nrd_all_modules.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_perf_counter.o: src/perf_counter/nacl_perf_counter.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_refcount.o: src/nacl_base/nacl_refcount.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/fault_injection.o: src/fault_injection/fault_injection.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_os_qualify.o: src/platform_qualify/linux/nacl_os_qualify.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/sysv_shm_and_mmap.o: src/platform_qualify/linux/sysv_shm_and_mmap.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify.o: src/platform_qualify/posix/nacl_dep_qualify.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_dep_qualify_arch.o: src/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_exit.o: src/platform/linux/nacl_exit.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_find_addrsp.o: src/platform/linux/nacl_find_addrsp.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_host_desc.o: src/platform/linux/nacl_host_desc.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_time.o: src/platform/linux/nacl_time.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_check.o: src/platform/nacl_check.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_host_desc_common.o: src/platform/nacl_host_desc_common.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_log.o: src/platform/nacl_log.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/platform_init.o: src/platform/platform_init.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/gio.o: src/gio/gio.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/gio_mem.o: src/gio/gio_mem.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/gprintf.o: src/gio/gprintf.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/gio_mem_snapshot.o: src/gio/gio_mem_snapshot.c
	@gcc ${CCFLAGS} ${CCFLAGS0} ${CCFLAGS1} -o $@ $^

obj/nacl_imc_c.o: src/imc/nacl_imc_c.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^

obj/nacl_imc_unistd.o: src/imc/nacl_imc_unistd.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^

obj/nacl_imc.o: src/imc/linux/nacl_imc.cc
	@g++ ${CXXFLAGS} ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -o $@ $^
