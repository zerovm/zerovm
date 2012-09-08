# ABS path is needed to correct gcov, gcov is used to get test coverage
#GCOV_FLAGS=--coverage -g3 -fprofile-arcs -ftest-coverage
#ABS_PATH=$(shell pwd)/

# todo(d'b): replace it with build switch
#RELEASE BUILD
#CCFLAGS=-DNDEBUG -O3
#CXXFLAGS=-DNDEBUG -O3

#DEBUG BUILD
CCFLAGS=-DDEBUG -g ${GCOV_FLAGS}
CXXFLAGS=-DDEBUG -g ${GCOV_FLAGS}

# todo(d'b): move it to proper place
NETW_LIB=-lzmq

CCFLAGS0=-c -m64 -fPIC -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare
CCFLAGS3=-fno-strict-aliasing -Wno-missing-field-initializers
CCFLAGS4=-DNACL_TRUSTED_BUT_NOT_TCB
CCFLAGS6=-DUNIT_TEST
CXXFLAGS1=-c -std=c++98 -Wno-variadic-macros -m64 -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4 -DNACL_TRUSTED_BUT_NOT_TCB -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CXXFLAGS2=-Wl,-z,noexecstack -m64 -Wno-variadic-macros -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=obj
CXXFLAGS3=-c -std=c++98 -Wno-variadic-macros -m64 -fPIE -Wall -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4 -DNACL_TRUSTED_BUT_NOT_TCB -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.

#all: create_dirs zerovm zvm_api ${NETW_MAIN_RULES} tests 
all: create_dirs zerovm zvm_api ${NETW_MAIN_RULES}

create_dirs: 
	@mkdir obj -p
	@mkdir test -p

zerovm: obj/sel_main.o obj/libsel.a obj/libnacl_error_code.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform_qual_lib.a obj/libplatform.a obj/libgio.a ${NETW_RULES}
	@g++ ${CXXFLAGS} -o zerovm ${CXXFLAGS2} obj/sel_main.o -L/usr/lib -lsel -lnacl_error_code -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lplatform_qual_lib -lplatform -lgio ${NETW_LIB} -lrt -lpthread -lcrypto -ldl -lglib-2.0 -Lobj -Lgtest  

gcov: clean all
	@lcov --directory . --base-directory=${ABS_PATH} --capture --output-file app.info
	@genhtml --output-directory cov_htmp app.info
	@echo run ${ABS_PATH}cov_htmp/index.html

#tests: test_compile 
#	@echo == unit test =================================================
#	test/service_runtime_tests
#	test/x86_decoder_tests_nc_inst_state
#	test/manifest_parser_test
#	test/manifest_setup_test
#	test/nacl_log_test

zvm_api: api/syscall_manager.S api/zrt.c api/zvm.c
	@make -Capi

#test_compile: test/service_runtime_tests test/x86_decoder_tests_nc_inst_state test/manifest_parser_test test/manifest_setup_test test/nacl_log_test ${NETW_TEST_RULES}

obj/manifest_parser_test.o: src/manifest/manifest_parser_test.cc
	@g++ ${CXXFLAGS} -o obj/manifest_parser_test.o ${CXXFLAGS1} src/manifest/manifest_parser_test.cc
test/manifest_parser_test: obj/manifest_parser_test.o obj/libsel.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/manifest_parser_test ${CXXFLAGS2} obj/manifest_parser_test.o -L/usr/lib -Lobj -Lgtest -lgtest -lsel -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lplatform -lgio ${NETW_LIB} -lrt -lpthread -lcrypto -ldl

obj/manifest_setup_test.o: src/manifest/manifest_setup_test.cc
	@g++ ${CXXFLAGS} -o obj/manifest_setup_test.o ${CXXFLAGS1} src/manifest/manifest_setup_test.cc
test/manifest_setup_test: obj/manifest_setup_test.o obj/libsel.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/manifest_setup_test ${CXXFLAGS2} obj/manifest_setup_test.o -L/usr/lib -Lobj -Lgtest -lgtest -lsel -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lplatform -lgio ${NETW_LIB} -lrt -lpthread -lcrypto -ldl

obj/nacl_log_test.o: src/platform/nacl_log_test.cc
	@g++ ${CXXFLAGS} -o obj/nacl_log_test.o ${CXXFLAGS1} src/platform/nacl_log_test.cc
test/nacl_log_test: obj/nacl_log_test.o obj/libsel.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/nacl_log_test ${CXXFLAGS2} obj/nacl_log_test.o -L/usr/lib -Lobj -Lgtest -lgtest -lsel -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lplatform -lgio ${NETW_LIB} -lrt -lpthread -lcrypto -ldl

obj/sel_ldr_test.o: src/service_runtime/sel_ldr_test.cc
	@g++ ${CXXFLAGS} -o obj/sel_ldr_test.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include src/service_runtime/sel_ldr_test.cc

obj/sel_mem_test.o: src/service_runtime/sel_mem_test.cc
	@g++ ${CXXFLAGS} -o obj/sel_mem_test.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include src/service_runtime/sel_mem_test.cc

obj/sel_memory_unittest.o: src/service_runtime/sel_memory_unittest.cc
	@g++ ${CXXFLAGS} -o obj/sel_memory_unittest.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include src/service_runtime/sel_memory_unittest.cc

obj/unittest_main.o: src/service_runtime/unittest_main.cc
	@g++ ${CXXFLAGS} -o obj/unittest_main.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include src/service_runtime/unittest_main.cc

test/service_runtime_tests: obj/sel_ldr_test.o obj/sel_mem_test.o obj/sel_memory_unittest.o obj/unittest_main.o obj/libsel.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/service_runtime_tests ${CXXFLAGS2} obj/unittest_main.o obj/sel_memory_unittest.o obj/sel_mem_test.o obj/sel_ldr_test.o -L/usr/lib -lgtest -lsel -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lplatform -lgio ${NETW_LIB} -lrt -lpthread -lcrypto -ldl -Lobj -Lgtest

test/x86_decoder_tests_nc_inst_state: obj/nc_inst_state_tests.o obj/ncval_decode_tables.o obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/x86_decoder_tests_nc_inst_state ${CXXFLAGS2} obj/nc_inst_state_tests.o obj/ncval_decode_tables.o -L/usr/lib -lgtest -lplatform -lgio -lrt -lpthread -lcrypto -Lobj -Lgtest


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
	@rm -f gtest/data/zerovm_test.db
	@rm -f zvm_netw.db
	
clean_api:
	@make -Capi clean
	@echo api binaries has been deleted

obj/libsel.a: obj/dyn_array.o obj/elf_util.o obj/nacl_all_modules.o obj/nacl_desc_effector_ldr.o obj/nacl_memory_object.o obj/nacl_signal_common.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/sel_addrspace.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_mem.o obj/sel_qualify.o obj/sel_util-inl.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr_x86_64.o obj/sel_rt_64.o obj/tramp_64.o obj/sel_addrspace_posix_x86_64.o obj/sel_memory.o obj/sel_segments.o obj/nacl_signal.o obj/nacl_signal_64.o obj/manifest_parser.o obj/manifest_setup.o obj/md5.o obj/mount_channel.o obj/prefetch.o obj/preload.o obj/trap.o obj/nacl_cpuid.o obj/nacl_xgetbv.o
	@ar rc obj/libsel.a obj/dyn_array.o obj/elf_util.o obj/nacl_all_modules.o obj/nacl_desc_effector_ldr.o obj/nacl_memory_object.o obj/nacl_signal_common.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/sel_addrspace.o obj/sel_ldr.o obj/sel_ldr_standard.o obj/sel_mem.o obj/sel_qualify.o obj/sel_util-inl.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr_x86_64.o obj/sel_rt_64.o obj/tramp_64.o obj/sel_addrspace_posix_x86_64.o obj/sel_memory.o obj/sel_segments.o obj/nacl_signal.o obj/nacl_signal_64.o obj/manifest_parser.o obj/manifest_setup.o obj/md5.o obj/mount_channel.o obj/prefetch.o obj/preload.o obj/trap.o obj/nacl_cpuid.o obj/nacl_xgetbv.o

obj/libnacl_error_code.a: obj/nacl_error_code.o
	@ar rc obj/libnacl_error_code.a obj/nacl_error_code.o

obj/libnrd_xfer.a: obj/nacl_desc_base.o obj/nacl_desc_imc_shm.o obj/nacl_desc_io.o obj/nrd_all_modules.o
	@ar rc obj/libnrd_xfer.a obj/nacl_desc_base.o obj/nacl_desc_imc_shm.o obj/nacl_desc_io.o obj/nrd_all_modules.o

obj/libnacl_perf_counter.a: obj/nacl_perf_counter.o
	@ar rc obj/libnacl_perf_counter.a obj/nacl_perf_counter.o

obj/libnacl_base.a: obj/nacl_refcount.o
	@ar rc obj/libnacl_base.a obj/nacl_refcount.o

obj/libimc.a: obj/nacl_imc_c.o obj/nacl_imc_unistd.o obj/nacl_imc.o
	@ar rc obj/libimc.a obj/nacl_imc_c.o obj/nacl_imc_unistd.o obj/nacl_imc.o

obj/libnacl_fault_inject.a: obj/fault_injection.o
	@ar rc obj/libnacl_fault_inject.a obj/fault_injection.o

obj/libplatform.a: obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc.o obj/nacl_time.o obj/nacl_check.o obj/nacl_host_desc_common.o obj/nacl_log.o obj/platform_init.o
	@ar rc obj/libplatform.a obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc.o obj/nacl_time.o obj/nacl_check.o obj/nacl_host_desc_common.o obj/nacl_log.o obj/platform_init.o

obj/libplatform_qual_lib.a: obj/nacl_os_qualify.o obj/sysv_shm_and_mmap.o obj/nacl_dep_qualify.o obj/nacl_dep_qualify_arch.o
	@ar rc obj/libplatform_qual_lib.a obj/nacl_os_qualify.o obj/sysv_shm_and_mmap.o obj/nacl_dep_qualify.o obj/nacl_dep_qualify_arch.o

obj/libgio.a: obj/gio.o obj/gio_mem.o obj/gprintf.o obj/gio_mem_snapshot.o
	@ar rc obj/libgio.a obj/gio.o obj/gio_mem.o obj/gprintf.o obj/gio_mem_snapshot.o

######################################################################## compilation to obj
obj/mount_channel.o: src/manifest/mount_channel.c
	@gcc ${CCFLAGS} -o obj/mount_channel.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} src/manifest/mount_channel.c

obj/prefetch.o: src/manifest/prefetch.c
	@gcc ${CCFLAGS} -o obj/prefetch.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} `pkg-config --cflags glib-2.0` src/manifest/prefetch.c

obj/preload.o: src/manifest/preload.c
	@gcc ${CCFLAGS} -o obj/preload.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} src/manifest/preload.c

obj/trap.o: src/manifest/trap.c
	@gcc ${CCFLAGS} -o obj/trap.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} src/manifest/trap.c

obj/manifest_setup.o: src/manifest/manifest_setup.c
	@gcc ${CCFLAGS} -o obj/manifest_setup.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} src/manifest/manifest_setup.c

obj/md5.o: src/manifest/md5.c
	@gcc ${CCFLAGS} -o obj/md5.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} src/manifest/md5.c

obj/manifest_parser.o: src/manifest/manifest_parser.c
	@gcc ${CCFLAGS} -o obj/manifest_parser.o ${CCFLAGS0} ${CCFLAGS1} src/manifest/manifest_parser.c

obj/nacl_switch_64.o: src/service_runtime/arch/x86_64/nacl_switch_64.S
	@gcc ${CCFLAGS} -o obj/nacl_switch_64.o ${CCFLAGS0} ${CCFLAGS2} src/service_runtime/arch/x86_64/nacl_switch_64.S

obj/nacl_syscall_64.o: src/service_runtime/arch/x86_64/nacl_syscall_64.S
	@gcc ${CCFLAGS} -o obj/nacl_syscall_64.o ${CCFLAGS0} ${CCFLAGS2} src/service_runtime/arch/x86_64/nacl_syscall_64.S

obj/tramp_64.o: src/service_runtime/arch/x86_64/tramp_64.S
	@gcc ${CCFLAGS} -o obj/tramp_64.o ${CCFLAGS0} ${CCFLAGS2} src/service_runtime/arch/x86_64/tramp_64.S

obj/nacl_cpuid.o: src/service_runtime/nacl_cpuid.c
	@gcc ${CCFLAGS} -o obj/nacl_cpuid.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_cpuid.c

obj/nacl_xgetbv.o: src/service_runtime/nacl_xgetbv.S
	@gcc ${CCFLAGS} -o obj/nacl_xgetbv.o ${CCFLAGS0} ${CCFLAGS2} src/service_runtime/nacl_xgetbv.S

obj/sel_main.o: src/service_runtime/sel_main.c
	@gcc ${CCFLAGS} -o obj/sel_main.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS5} `pkg-config --cflags glib-2.0` src/service_runtime/sel_main.c

obj/dyn_array.o: src/service_runtime/dyn_array.c
	@gcc ${CCFLAGS} -o obj/dyn_array.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/dyn_array.c

obj/elf_util.o: src/service_runtime/elf_util.c
	@gcc ${CCFLAGS} -o obj/elf_util.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/elf_util.c

obj/nacl_all_modules.o: src/service_runtime/nacl_all_modules.c
	@gcc ${CCFLAGS} -o obj/nacl_all_modules.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_all_modules.c

obj/nacl_desc_effector_ldr.o: src/service_runtime/nacl_desc_effector_ldr.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_effector_ldr.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_desc_effector_ldr.c

obj/nacl_memory_object.o: src/service_runtime/nacl_memory_object.c
	@gcc ${CCFLAGS} -o obj/nacl_memory_object.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_memory_object.c

obj/nacl_signal_common.o: src/service_runtime/nacl_signal_common.c
	@gcc ${CCFLAGS} -o obj/nacl_signal_common.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_signal_common.c

obj/nacl_syscall_handlers.o: src/service_runtime/nacl_syscall_handlers.c
	@gcc ${CCFLAGS} -o obj/nacl_syscall_handlers.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_syscall_handlers.c

obj/nacl_syscall_hook.o: src/service_runtime/nacl_syscall_hook.c
	@gcc ${CCFLAGS} -o obj/nacl_syscall_hook.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_syscall_hook.c

obj/nacl_text.o: src/service_runtime/nacl_text.c
	@gcc ${CCFLAGS} -o obj/nacl_text.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_text.c

obj/sel_addrspace.o: src/service_runtime/sel_addrspace.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_addrspace.c

obj/sel_ldr.o: src/service_runtime/sel_ldr.c
	@gcc ${CCFLAGS} -o obj/sel_ldr.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_ldr.c

obj/sel_ldr_standard.o: src/service_runtime/sel_ldr_standard.c
	@gcc ${CCFLAGS} -o obj/sel_ldr_standard.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_ldr_standard.c

obj/sel_mem.o: src/service_runtime/sel_mem.c
	@gcc ${CCFLAGS} -o obj/sel_mem.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_mem.c

obj/sel_qualify.o: src/service_runtime/sel_qualify.c
	@gcc ${CCFLAGS} -o obj/sel_qualify.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_qualify.c

obj/sel_util-inl.o: src/service_runtime/sel_util-inl.c
	@gcc ${CCFLAGS} -o obj/sel_util-inl.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/sel_util-inl.c

obj/nacl_switch_to_app_64.o: src/service_runtime/arch/x86_64/nacl_switch_to_app_64.c
	@gcc ${CCFLAGS} -o obj/nacl_switch_to_app_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/arch/x86_64/nacl_switch_to_app_64.c

obj/sel_addrspace_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace_x86_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/arch/x86_64/sel_addrspace_x86_64.c

obj/sel_ldr_x86_64.o: src/service_runtime/arch/x86_64/sel_ldr_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_ldr_x86_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/arch/x86_64/sel_ldr_x86_64.c

obj/sel_rt_64.o: src/service_runtime/arch/x86_64/sel_rt_64.c
	@gcc ${CCFLAGS} -o obj/sel_rt_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/arch/x86_64/sel_rt_64.c

obj/sel_addrspace_posix_x86_64.o: src/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace_posix_x86_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c

obj/sel_memory.o: src/service_runtime/linux/sel_memory.c
	@gcc ${CCFLAGS} -o obj/sel_memory.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/linux/sel_memory.c

obj/sel_segments.o: src/service_runtime/linux/x86/sel_segments.c
	@gcc ${CCFLAGS} -o obj/sel_segments.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/linux/x86/sel_segments.c

obj/nacl_signal.o: src/service_runtime/posix/nacl_signal.c
	@gcc ${CCFLAGS} -o obj/nacl_signal.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/posix/nacl_signal.c

obj/nacl_signal_64.o: src/service_runtime/linux/nacl_signal_64.c
	@gcc ${CCFLAGS} -o obj/nacl_signal_64.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/linux/nacl_signal_64.c

obj/nacl_error_code.o: src/service_runtime/nacl_error_code.c
	@gcc ${CCFLAGS} -o obj/nacl_error_code.o ${CCFLAGS0} ${CCFLAGS1} src/service_runtime/nacl_error_code.c

obj/nacl_desc_base.o: src/desc/nacl_desc_base.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_base.o ${CCFLAGS0} ${CCFLAGS1} src/desc/nacl_desc_base.c

obj/nacl_desc_imc_shm.o: src/desc/nacl_desc_imc_shm.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_imc_shm.o ${CCFLAGS0} ${CCFLAGS1} src/desc/nacl_desc_imc_shm.c

obj/nacl_desc_io.o: src/desc/nacl_desc_io.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_io.o ${CCFLAGS0} ${CCFLAGS1} src/desc/nacl_desc_io.c

obj/nrd_all_modules.o: src/desc/nrd_all_modules.c
	@gcc ${CCFLAGS} -o obj/nrd_all_modules.o ${CCFLAGS0} ${CCFLAGS1} src/desc/nrd_all_modules.c

obj/nacl_perf_counter.o: src/perf_counter/nacl_perf_counter.c
	@gcc ${CCFLAGS} -o obj/nacl_perf_counter.o ${CCFLAGS0} ${CCFLAGS1} src/perf_counter/nacl_perf_counter.c

obj/nacl_refcount.o: src/nacl_base/nacl_refcount.c
	@gcc ${CCFLAGS} -o obj/nacl_refcount.o ${CCFLAGS0} ${CCFLAGS1} src/nacl_base/nacl_refcount.c

obj/fault_injection.o: src/fault_injection/fault_injection.c
	@gcc ${CCFLAGS} -o obj/fault_injection.o ${CCFLAGS0} ${CCFLAGS1} src/fault_injection/fault_injection.c

obj/nacl_os_qualify.o: src/platform_qualify/linux/nacl_os_qualify.c
	@gcc ${CCFLAGS} -o obj/nacl_os_qualify.o ${CCFLAGS0} ${CCFLAGS1} src/platform_qualify/linux/nacl_os_qualify.c

obj/sysv_shm_and_mmap.o: src/platform_qualify/linux/sysv_shm_and_mmap.c
	@gcc ${CCFLAGS} -o obj/sysv_shm_and_mmap.o ${CCFLAGS0} ${CCFLAGS1} src/platform_qualify/linux/sysv_shm_and_mmap.c

obj/nacl_dep_qualify.o: src/platform_qualify/posix/nacl_dep_qualify.c
	@gcc ${CCFLAGS} -o obj/nacl_dep_qualify.o ${CCFLAGS0} ${CCFLAGS1} src/platform_qualify/posix/nacl_dep_qualify.c

obj/nacl_dep_qualify_arch.o: src/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c
	@gcc ${CCFLAGS} -o obj/nacl_dep_qualify_arch.o ${CCFLAGS0} ${CCFLAGS1} src/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c

obj/nacl_exit.o: src/platform/linux/nacl_exit.c
	@gcc ${CCFLAGS} -o obj/nacl_exit.o ${CCFLAGS0} ${CCFLAGS1} src/platform/linux/nacl_exit.c

obj/nacl_find_addrsp.o: src/platform/linux/nacl_find_addrsp.c
	@gcc ${CCFLAGS} -o obj/nacl_find_addrsp.o ${CCFLAGS0} ${CCFLAGS1} src/platform/linux/nacl_find_addrsp.c

obj/nacl_host_desc.o: src/platform/linux/nacl_host_desc.c
	@gcc ${CCFLAGS} -o obj/nacl_host_desc.o ${CCFLAGS0} ${CCFLAGS1} src/platform/linux/nacl_host_desc.c

obj/nacl_time.o: src/platform/linux/nacl_time.c
	@gcc ${CCFLAGS} -o obj/nacl_time.o ${CCFLAGS0} ${CCFLAGS1} src/platform/linux/nacl_time.c

obj/nacl_check.o: src/platform/nacl_check.c
	@gcc ${CCFLAGS} -o obj/nacl_check.o ${CCFLAGS0} ${CCFLAGS1} src/platform/nacl_check.c

obj/nacl_host_desc_common.o: src/platform/nacl_host_desc_common.c
	@gcc ${CCFLAGS} -o obj/nacl_host_desc_common.o ${CCFLAGS0} ${CCFLAGS1} src/platform/nacl_host_desc_common.c

obj/nacl_log.o: src/platform/nacl_log.c
	@gcc ${CCFLAGS} -o obj/nacl_log.o ${CCFLAGS0} ${CCFLAGS1} src/platform/nacl_log.c

obj/platform_init.o: src/platform/platform_init.c
	@gcc ${CCFLAGS} -o obj/platform_init.o ${CCFLAGS0} ${CCFLAGS1} src/platform/platform_init.c

obj/gio.o: src/gio/gio.c
	@gcc ${CCFLAGS} -o obj/gio.o ${CCFLAGS0} ${CCFLAGS1} src/gio/gio.c

obj/gio_mem.o: src/gio/gio_mem.c
	@gcc ${CCFLAGS} -o obj/gio_mem.o ${CCFLAGS0} ${CCFLAGS1} src/gio/gio_mem.c

obj/gprintf.o: src/gio/gprintf.c
	@gcc ${CCFLAGS} -o obj/gprintf.o ${CCFLAGS0} ${CCFLAGS1} src/gio/gprintf.c

obj/gio_mem_snapshot.o: src/gio/gio_mem_snapshot.c
	@gcc ${CCFLAGS} -o obj/gio_mem_snapshot.o ${CCFLAGS0} ${CCFLAGS1} src/gio/gio_mem_snapshot.c

obj/nacl_imc_c.o: src/imc/nacl_imc_c.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc_c.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} src/imc/nacl_imc_c.cc

obj/nacl_imc_unistd.o: src/imc/nacl_imc_unistd.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc_unistd.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} src/imc/nacl_imc_unistd.cc

obj/nacl_imc.o: src/imc/linux/nacl_imc.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} src/imc/linux/nacl_imc.cc

