#RELEASE BUILD
#CCFLAGS=-DNDEBUG -O2
#CXXFLAGS=-DNDEBUG -O2

#DEBUG BUILD
CCFLAGS=-DDEBUG -g
CXXFLAGS=-DDEBUG -g

CCFLAGS0=-c -m64 -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CCFLAGS1=-std=gnu99 -Wdeclaration-after-statement -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4
CCFLAGS2=-Wextra -Wswitch-enum -Wsign-compare
CCFLAGS3=-fno-strict-aliasing -Wno-missing-field-initializers
CCFLAGS4=-DNACL_TRUSTED_BUT_NOT_TCB
CXXFLAGS1=-c -std=c++98 -Wno-variadic-macros -m64 -fPIE -Wall -pedantic -Wno-long-long -fvisibility=hidden -fstack-protector --param ssp-buffer-size=4 -DNACL_TRUSTED_BUT_NOT_TCB -D_FORTIFY_SOURCE=2 -DNACL_WINDOWS=0 -DNACL_OSX=0 -DNACL_LINUX=1 -D_BSD_SOURCE=1 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE=600 -D_GNU_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1 -DNACL_BLOCK_SHIFT=5 -DNACL_BLOCK_SIZE=32 -DNACL_BUILD_ARCH=x86 -DNACL_BUILD_SUBARCH=64 -DNACL_TARGET_ARCH=x86 -DNACL_TARGET_SUBARCH=64 -DNACL_STANDALONE=1 -DNACL_ENABLE_TMPFS_REDIRECT_VAR=0 -I.
CXXFLAGS2=-Wl,-z,noexecstack -m64 -Wno-variadic-macros -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=obj

all: zerovm tests

zerovm: obj/sel_main.o obj/libsel.a obj/libenv_cleanser.a obj/libnacl_error_code.a obj/libmanifest_proxy.a obj/libsimple_service.a obj/libthread_interface.a obj/libgio_wrapped_desc.a obj/libnonnacl_srpc.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libcontainer.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libplatform_qual_lib.a obj/libncvalidate_x86_64.a obj/libncval_reg_sfi_x86_64.a obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a obj/libdebug_stub_init.a obj/libgdb_rsp.a obj/libdebug_stub.a
	@g++ ${CXXFLAGS} -o zerovm ${CXXFLAGS2} obj/sel_main.o -L/usr/lib -lsel -lenv_cleanser -lnacl_error_code -lmanifest_proxy -lsimple_service -lthread_interface -lgio_wrapped_desc -lnonnacl_srpc -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lcontainer -lnacl_fault_inject -lplatform -lplatform_qual_lib -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -ldebug_stub_init -lgdb_rsp -ldebug_stub -lrt -lpthread -lcrypto -ldl -Lobj -Lgtest

tests: test_compile
	test/x86_validator_tests_nc_remaining_memory
	test/service_runtime_tests
	test/x86_decoder_tests_nc_inst_state
	test/x86_validator_tests_halt_trim
	test/x86_validator_tests_nc_inst_bytes

test_compile: test/x86_validator_tests_halt_trim test/service_runtime_tests test/x86_decoder_tests_nc_inst_state test/x86_validator_tests_nc_inst_bytes test/x86_validator_tests_nc_remaining_memory

obj/halt_trim_tests.o: trusted/validator/x86/halt_trim_tests.cc
	@g++ ${CXXFLAGS} -o obj/halt_trim_tests.o ${CXXFLAGS1} -Igtest/include trusted/validator/x86/halt_trim_tests.cc

test/x86_validator_tests_halt_trim: obj/halt_trim_tests.o obj/libncvalidate_x86_64.a obj/libncval_reg_sfi_x86_64.a obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/x86_validator_tests_halt_trim ${CXXFLAGS2} obj/halt_trim_tests.o -L/usr/lib -Lobj -Lgtest -lgtest -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -lrt -lpthread -lcrypto

obj/sel_ldr_test.o: trusted/service_runtime/sel_ldr_test.cc
	@g++ ${CXXFLAGS} -o obj/sel_ldr_test.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include trusted/service_runtime/sel_ldr_test.cc

obj/sel_mem_test.o: trusted/service_runtime/sel_mem_test.cc
	@g++ ${CXXFLAGS} -o obj/sel_mem_test.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include trusted/service_runtime/sel_mem_test.cc

obj/sel_memory_unittest.o: trusted/service_runtime/sel_memory_unittest.cc
	@g++ ${CXXFLAGS} -o obj/sel_memory_unittest.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include trusted/service_runtime/sel_memory_unittest.cc

obj/unittest_main.o: trusted/service_runtime/unittest_main.cc
	@g++ ${CXXFLAGS} -o obj/unittest_main.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -Igtest/include trusted/service_runtime/unittest_main.cc

test/service_runtime_tests: obj/sel_ldr_test.o obj/sel_mem_test.o obj/sel_memory_unittest.o obj/unittest_main.o obj/libsel.a obj/libenv_cleanser.a obj/libmanifest_proxy.a obj/libsimple_service.a obj/libthread_interface.a obj/libgio_wrapped_desc.a obj/libnonnacl_srpc.a obj/libnrd_xfer.a obj/libnacl_perf_counter.a obj/libnacl_base.a obj/libimc.a obj/libnacl_fault_inject.a obj/libplatform.a obj/libncvalidate_x86_64.a obj/libncval_reg_sfi_x86_64.a obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a obj/libdebug_stub_init.a obj/libgdb_rsp.a obj/libdebug_stub.a
	@g++ ${CXXFLAGS} -o test/service_runtime_tests ${CXXFLAGS2} obj/unittest_main.o obj/sel_memory_unittest.o obj/sel_mem_test.o obj/sel_ldr_test.o -L/usr/lib -lgtest -lsel -lenv_cleanser -lmanifest_proxy -lsimple_service -lthread_interface -lgio_wrapped_desc -lnonnacl_srpc -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lnacl_fault_inject -lplatform -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -ldebug_stub_init -lgdb_rsp -ldebug_stub -lrt -lpthread -lcrypto -ldl -Lobj -Lgtest

obj/nc_inst_state_tests.o: trusted/validator/x86/decoder/nc_inst_state_tests.cc
	@g++ ${CXXFLAGS} -o obj/nc_inst_state_tests.o ${CXXFLAGS1} -Igtest/include trusted/validator/x86/decoder/nc_inst_state_tests.cc

test/x86_decoder_tests_nc_inst_state: obj/nc_inst_state_tests.o obj/ncval_decode_tables.o obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/x86_decoder_tests_nc_inst_state ${CXXFLAGS2} obj/nc_inst_state_tests.o obj/ncval_decode_tables.o -L/usr/lib -lgtest -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -lrt -lpthread -lcrypto -Lobj -Lgtest

obj/nc_inst_bytes_tests.o: trusted/validator/x86/nc_inst_bytes_tests.cc
	@g++ ${CXXFLAGS} -o obj/nc_inst_bytes_tests.o ${CXXFLAGS1} -Igtest/include trusted/validator/x86/nc_inst_bytes_tests.cc

test/x86_validator_tests_nc_inst_bytes: obj/nc_inst_bytes_tests.o obj/libncvalidate_x86_64.a obj/libncval_reg_sfi_x86_64.a obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/x86_validator_tests_nc_inst_bytes ${CXXFLAGS2} obj/nc_inst_bytes_tests.o -L/usr/lib -lgtest -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -lrt -lpthread -lcrypto -Lobj -Lgtest

obj/nc_remaining_memory_tests.o: trusted/validator/x86/nc_remaining_memory_tests.cc
	@g++ ${CXXFLAGS} -o obj/nc_remaining_memory_tests.o ${CXXFLAGS1} -Igtest/include trusted/validator/x86/nc_remaining_memory_tests.cc

test/x86_validator_tests_nc_remaining_memory: obj/nc_remaining_memory_tests.o obj/libncvalidate_x86_64.a obj/libncval_reg_sfi_x86_64.a obj/libnccopy_x86_64.a obj/libnc_decoder_x86_64.a obj/libnc_opcode_modeling_x86_64.a obj/libncval_base_x86_64.a obj/libplatform.a obj/libgio.a
	@g++ ${CXXFLAGS} -o test/x86_validator_tests_nc_remaining_memory ${CXXFLAGS2} obj/nc_remaining_memory_tests.o -L/usr/lib -lgtest -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -lrt -lpthread -lcrypto -Lobj -Lgtest

clean: clean_intermediate
	@rm zerovm
	@echo ZERO VM has been deleted

clean_intermediate:
	@rm obj/*
	@echo intermediate files has been deleted
	@rm test/*
	@echo unit tests has been deleted

obj/libsel.a: obj/dyn_array.o obj/elf_util.o obj/nacl_all_modules.o obj/nacl_app_thread.o obj/nacl_desc_effector_ldr.o obj/nacl_globals.o obj/nacl_kern_services.o obj/nacl_memory_object.o obj/nacl_signal_common.o obj/nacl_stack_safety.o obj/nacl_syscall_common.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/nacl_valgrind_hooks.o obj/default_name_service.o obj/name_service.o obj/sel_addrspace.o obj/sel_ldr.o obj/sel_ldr-inl.o obj/sel_ldr_standard.o obj/sel_ldr_thread_interface.o obj/sel_mem.o obj/sel_qualify.o obj/sel_util-inl.o obj/sel_validate_image.o obj/nacl_ldt_x86.o obj/nacl_app_64.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_tls_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr_x86_64.o obj/sel_rt_64.o obj/tramp_64.o obj/sel_addrspace_posix_x86_64.o obj/sel_memory.o obj/nacl_oop_debugger_hooks.o obj/nacl_thread_nice.o obj/nacl_ldt.o obj/sel_segments.o obj/nacl_signal.o obj/nacl_signal_64.o obj/zmq_syscalls.o obj/manifest_parse.o
	@ar rc obj/libsel.a obj/dyn_array.o obj/elf_util.o obj/nacl_all_modules.o obj/nacl_app_thread.o obj/nacl_desc_effector_ldr.o obj/nacl_globals.o obj/nacl_kern_services.o obj/nacl_memory_object.o obj/nacl_signal_common.o obj/nacl_stack_safety.o obj/nacl_syscall_common.o obj/nacl_syscall_handlers.o obj/nacl_syscall_hook.o obj/nacl_text.o obj/nacl_valgrind_hooks.o obj/default_name_service.o obj/name_service.o obj/sel_addrspace.o obj/sel_ldr.o obj/sel_ldr-inl.o obj/sel_ldr_standard.o obj/sel_ldr_thread_interface.o obj/sel_mem.o obj/sel_qualify.o obj/sel_util-inl.o obj/sel_validate_image.o obj/nacl_ldt_x86.o obj/nacl_app_64.o obj/nacl_switch_64.o obj/nacl_switch_to_app_64.o obj/nacl_syscall_64.o obj/nacl_tls_64.o obj/sel_addrspace_x86_64.o obj/sel_ldr_x86_64.o obj/sel_rt_64.o obj/tramp_64.o obj/sel_addrspace_posix_x86_64.o obj/sel_memory.o obj/nacl_oop_debugger_hooks.o obj/nacl_thread_nice.o obj/nacl_ldt.o obj/sel_segments.o obj/nacl_signal.o obj/nacl_signal_64.o obj/zmq_syscalls.o obj/manifest_parse.o

obj/libenv_cleanser.a: obj/env_cleanser.o
	@ar rc obj/libenv_cleanser.a obj/env_cleanser.o

obj/libnacl_error_code.a: obj/nacl_error_code.o
	@ar rc obj/libnacl_error_code.a obj/nacl_error_code.o

obj/libmanifest_proxy.a: obj/manifest_proxy.o
	@ar rc obj/libmanifest_proxy.a obj/manifest_proxy.o

obj/libsimple_service.a: obj/nacl_simple_ltd_service.o obj/nacl_simple_rservice.o obj/nacl_simple_service.o
	@ar rc obj/libsimple_service.a obj/nacl_simple_ltd_service.o obj/nacl_simple_rservice.o obj/nacl_simple_service.o

obj/libthread_interface.a: obj/nacl_thread_interface.o
	@ar rc obj/libthread_interface.a obj/nacl_thread_interface.o

obj/libgio_wrapped_desc.a: obj/gio_shm.o obj/gio_shm_unbounded.o obj/gio_nacl_desc.o
	@ar rc obj/libgio_wrapped_desc.a obj/gio_shm.o obj/gio_shm_unbounded.o obj/gio_nacl_desc.o

obj/libnonnacl_srpc.a: obj/invoke.o obj/module_init_fini.o obj/nacl_srpc.o obj/nacl_srpc_message.o obj/rpc_log.o obj/rpc_serialize.o obj/rpc_service.o obj/rpc_server_loop.o
	@ar rc obj/libnonnacl_srpc.a obj/invoke.o obj/module_init_fini.o obj/nacl_srpc.o obj/nacl_srpc_message.o obj/rpc_log.o obj/rpc_serialize.o obj/rpc_service.o obj/rpc_server_loop.o

obj/libnrd_xfer.a: obj/nacl_desc_base.o obj/nacl_desc_cond.o obj/nacl_desc_dir.o obj/nacl_desc_effector_trusted_mem.o obj/nacl_desc_imc.o obj/nacl_desc_imc_shm.o obj/nacl_desc_invalid.o obj/nacl_desc_io.o obj/nacl_desc_mutex.o obj/nacl_desc_rng.o obj/nacl_desc_quota.o obj/nacl_desc_semaphore.o obj/nacl_desc_sync_socket.o obj/nacl_pepper.o obj/nrd_all_modules.o obj/nrd_xfer.o obj/nrd_xfer_effector.o obj/nacl_desc_wrapper.o obj/nacl_desc.o obj/nacl_desc_sysv_shm.o obj/nacl_desc_conn_cap.o obj/nacl_desc_imc_bound_desc.o obj/nacl_makeboundsock.o
	@ar rc obj/libnrd_xfer.a obj/nacl_desc_base.o obj/nacl_desc_cond.o obj/nacl_desc_dir.o obj/nacl_desc_effector_trusted_mem.o obj/nacl_desc_imc.o obj/nacl_desc_imc_shm.o obj/nacl_desc_invalid.o obj/nacl_desc_io.o obj/nacl_desc_mutex.o obj/nacl_desc_rng.o obj/nacl_desc_quota.o obj/nacl_desc_semaphore.o obj/nacl_desc_sync_socket.o obj/nacl_pepper.o obj/nrd_all_modules.o obj/nrd_xfer.o obj/nrd_xfer_effector.o obj/nacl_desc_wrapper.o obj/nacl_desc.o obj/nacl_desc_sysv_shm.o obj/nacl_desc_conn_cap.o obj/nacl_desc_imc_bound_desc.o obj/nacl_makeboundsock.o

obj/libnacl_perf_counter.a: obj/nacl_perf_counter.o
	@ar rc obj/libnacl_perf_counter.a obj/nacl_perf_counter.o

obj/libnacl_base.a: obj/nacl_refcount.o
	@ar rc obj/libnacl_base.a obj/nacl_refcount.o

obj/libimc.a: obj/nacl_imc_common.o obj/nacl_imc_c.o obj/nacl_imc_unistd.o obj/nacl_imc.o
	@ar rc obj/libimc.a obj/nacl_imc_common.o obj/nacl_imc_c.o obj/nacl_imc_unistd.o obj/nacl_imc.o

obj/libcontainer.a: obj/container.o
	@ar rc obj/libcontainer.a obj/container.o

obj/libnacl_fault_inject.a: obj/fault_injection.o
	@ar rc obj/libnacl_fault_inject.a obj/fault_injection.o

obj/libplatform.a: obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc.o obj/nacl_host_dir.o obj/nacl_secure_random.o obj/nacl_semaphore.o obj/nacl_thread_id.o obj/nacl_threads.o obj/nacl_time.o obj/nacl_timestamp.o obj/condition_variable.o obj/lock.o obj/nacl_check.o obj/nacl_global_secure_random.o obj/nacl_host_desc_common.o obj/nacl_interruptible_condvar.o obj/nacl_interruptible_mutex.o obj/nacl_log.o obj/nacl_secure_random_common.o obj/nacl_sync_checked.o obj/nacl_time_common.o obj/platform_init.o obj/refcount_base.o
	@ar rc obj/libplatform.a obj/nacl_exit.o obj/nacl_find_addrsp.o obj/nacl_host_desc.o obj/nacl_host_dir.o obj/nacl_secure_random.o obj/nacl_semaphore.o obj/nacl_thread_id.o obj/nacl_threads.o obj/nacl_time.o obj/nacl_timestamp.o obj/condition_variable.o obj/lock.o obj/nacl_check.o obj/nacl_global_secure_random.o obj/nacl_host_desc_common.o obj/nacl_interruptible_condvar.o obj/nacl_interruptible_mutex.o obj/nacl_log.o obj/nacl_secure_random_common.o obj/nacl_sync_checked.o obj/nacl_time_common.o obj/platform_init.o obj/refcount_base.o

obj/libplatform_qual_lib.a: obj/nacl_os_qualify.o obj/sysv_shm_and_mmap.o obj/nacl_dep_qualify.o obj/nacl_cpuwhitelist.o obj/nacl_dep_qualify_arch.o
	@ar rc obj/libplatform_qual_lib.a obj/nacl_os_qualify.o obj/sysv_shm_and_mmap.o obj/nacl_dep_qualify.o obj/nacl_cpuwhitelist.o obj/nacl_dep_qualify_arch.o

obj/libncvalidate_x86_64.a: obj/ncvalidate.o
	@ar rc obj/libncvalidate_x86_64.a obj/ncvalidate.o

obj/libncval_reg_sfi_x86_64.a: obj/ncvalidate_iter.o obj/ncvalidate_iter_detailed.o obj/ncvalidator_registry.o obj/ncvalidator_registry_detailed.o obj/nc_cpu_checks.o obj/nc_illegal.o obj/nc_jumps.o obj/address_sets.o obj/nc_jumps_detailed.o obj/nc_opcode_histogram.o obj/nc_protect_base.o obj/nc_memory_protect.o obj/ncvalidate_utils.o obj/ncval_decode_tables.o
	@ar rc obj/libncval_reg_sfi_x86_64.a obj/ncvalidate_iter.o obj/ncvalidate_iter_detailed.o obj/ncvalidator_registry.o obj/ncvalidator_registry_detailed.o obj/nc_cpu_checks.o obj/nc_illegal.o obj/nc_jumps.o obj/address_sets.o obj/nc_jumps_detailed.o obj/nc_opcode_histogram.o obj/nc_protect_base.o obj/nc_memory_protect.o obj/ncvalidate_utils.o obj/ncval_decode_tables.o

obj/libnccopy_x86_64.a: obj/nccopycode.o obj/nccopycode_stores.o
	@ar rc obj/libnccopy_x86_64.a obj/nccopycode.o obj/nccopycode_stores.o

obj/libnc_decoder_x86_64.a: obj/nc_inst_iter.o obj/nc_inst_state.o obj/nc_inst_trans.o obj/ncop_exps.o
	@ar rc obj/libnc_decoder_x86_64.a obj/nc_inst_iter.o obj/nc_inst_state.o obj/nc_inst_trans.o obj/ncop_exps.o

obj/libnc_opcode_modeling_x86_64.a: obj/ncopcode_desc.o
	@ar rc obj/libnc_opcode_modeling_x86_64.a obj/ncopcode_desc.o

obj/libncval_base_x86_64.a: obj/error_reporter.o obj/halt_trim.o obj/nacl_cpuid.o obj/nacl_xgetbv.o obj/ncinstbuffer.o obj/x86_insts.o obj/nc_segment.o
	@ar rc obj/libncval_base_x86_64.a obj/error_reporter.o obj/halt_trim.o obj/nacl_cpuid.o obj/nacl_xgetbv.o obj/ncinstbuffer.o obj/x86_insts.o obj/nc_segment.o

obj/libgio.a: obj/gio.o obj/gio_mem.o obj/gprintf.o obj/gio_mem_snapshot.o
	@ar rc obj/libgio.a obj/gio.o obj/gio_mem.o obj/gprintf.o obj/gio_mem_snapshot.o

obj/libdebug_stub_init.a: obj/nacl_debug.o
	@ar rc obj/libdebug_stub_init.a obj/nacl_debug.o

obj/libgdb_rsp.a: obj/abi.o obj/host.o obj/packet.o obj/session.o obj/target.o obj/util.o
	@ar rc obj/libgdb_rsp.a obj/abi.o obj/host.o obj/packet.o obj/session.o obj/target.o obj/util.o

obj/libdebug_stub.a: obj/debug_stub.o obj/event_common.o obj/platform_common.o obj/transport_common.o obj/debug_stub_posix.o obj/mutex_impl.o obj/platform_impl.o obj/thread_impl.o
	@ar rc obj/libdebug_stub.a obj/debug_stub.o obj/event_common.o obj/platform_common.o obj/transport_common.o obj/debug_stub_posix.o obj/mutex_impl.o obj/platform_impl.o obj/thread_impl.o

obj/ncvalidate.o: trusted/validator/x86/64/ncvalidate.c
	@gcc ${CCFLAGS} -o obj/ncvalidate.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS4} trusted/validator/x86/64/ncvalidate.c

obj/nrd_xfer.o: trusted/desc/nrd_xfer.c
	@gcc ${CCFLAGS} -o obj/nrd_xfer.o ${CCFLAGS0} ${CCFLAGS1} ${CCFLAGS3} trusted/desc/nrd_xfer.c

obj/nacl_switch_64.o: trusted/service_runtime/arch/x86_64/nacl_switch_64.S
	@gcc ${CCFLAGS} -o obj/nacl_switch_64.o ${CCFLAGS0} ${CCFLAGS2} trusted/service_runtime/arch/x86_64/nacl_switch_64.S

obj/nacl_syscall_64.o: trusted/service_runtime/arch/x86_64/nacl_syscall_64.S
	@gcc ${CCFLAGS} -o obj/nacl_syscall_64.o ${CCFLAGS0} ${CCFLAGS2} trusted/service_runtime/arch/x86_64/nacl_syscall_64.S

obj/tramp_64.o: trusted/service_runtime/arch/x86_64/tramp_64.S
	@gcc ${CCFLAGS} -o obj/tramp_64.o ${CCFLAGS0} ${CCFLAGS2} trusted/service_runtime/arch/x86_64/tramp_64.S

obj/nccopycode_stores.o: trusted/validator_x86/nccopycode_stores.S
	@gcc ${CCFLAGS} -o obj/nccopycode_stores.o ${CCFLAGS0} ${CCFLAGS2} trusted/validator_x86/nccopycode_stores.S

obj/nacl_xgetbv.o: trusted/validator/x86/nacl_xgetbv.S
	@gcc ${CCFLAGS} -o obj/nacl_xgetbv.o ${CCFLAGS0} ${CCFLAGS2} trusted/validator/x86/nacl_xgetbv.S

obj/manifest_parse.o: trusted/service_runtime/manifest_parse.c
	@gcc ${CCFLAGS} -o obj/manifest_parse.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/manifest_parse.c

obj/zmq_syscalls.o: trusted/service_runtime/zmq_syscalls.c
	@gcc ${CCFLAGS} -o obj/zmq_syscalls.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/zmq_syscalls.c

obj/sel_main.o: trusted/service_runtime/sel_main.c
	@gcc ${CCFLAGS} -o obj/sel_main.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_main.c

obj/dyn_array.o: trusted/service_runtime/dyn_array.c
	@gcc ${CCFLAGS} -o obj/dyn_array.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/dyn_array.c

obj/elf_util.o: trusted/service_runtime/elf_util.c
	@gcc ${CCFLAGS} -o obj/elf_util.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/elf_util.c

obj/nacl_all_modules.o: trusted/service_runtime/nacl_all_modules.c
	@gcc ${CCFLAGS} -o obj/nacl_all_modules.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_all_modules.c

obj/nacl_app_thread.o: trusted/service_runtime/nacl_app_thread.c
	@gcc ${CCFLAGS} -o obj/nacl_app_thread.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_app_thread.c

obj/nacl_desc_effector_ldr.o: trusted/service_runtime/nacl_desc_effector_ldr.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_effector_ldr.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_desc_effector_ldr.c

obj/nacl_globals.o: trusted/service_runtime/nacl_globals.c
	@gcc ${CCFLAGS} -o obj/nacl_globals.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_globals.c

obj/nacl_kern_services.o: trusted/service_runtime/nacl_kern_services.c
	@gcc ${CCFLAGS} -o obj/nacl_kern_services.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_kern_services.c

obj/nacl_memory_object.o: trusted/service_runtime/nacl_memory_object.c
	@gcc ${CCFLAGS} -o obj/nacl_memory_object.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_memory_object.c

obj/nacl_signal_common.o: trusted/service_runtime/nacl_signal_common.c
	@gcc ${CCFLAGS} -o obj/nacl_signal_common.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_signal_common.c

obj/nacl_stack_safety.o: trusted/service_runtime/nacl_stack_safety.c
	@gcc ${CCFLAGS} -o obj/nacl_stack_safety.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_stack_safety.c

obj/nacl_syscall_common.o: trusted/service_runtime/nacl_syscall_common.c
	@gcc ${CCFLAGS} -o obj/nacl_syscall_common.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_syscall_common.c

obj/nacl_syscall_handlers.o: trusted/service_runtime/nacl_syscall_handlers.c
	@gcc ${CCFLAGS} -o obj/nacl_syscall_handlers.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_syscall_handlers.c

obj/nacl_syscall_hook.o: trusted/service_runtime/nacl_syscall_hook.c
	@gcc ${CCFLAGS} -o obj/nacl_syscall_hook.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_syscall_hook.c

obj/nacl_text.o: trusted/service_runtime/nacl_text.c
	@gcc ${CCFLAGS} -o obj/nacl_text.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_text.c

obj/nacl_valgrind_hooks.o: trusted/service_runtime/nacl_valgrind_hooks.c
	@gcc ${CCFLAGS} -o obj/nacl_valgrind_hooks.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_valgrind_hooks.c

obj/default_name_service.o: trusted/service_runtime/name_service/default_name_service.c
	@gcc ${CCFLAGS} -o obj/default_name_service.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/name_service/default_name_service.c

obj/name_service.o: trusted/service_runtime/name_service/name_service.c
	@gcc ${CCFLAGS} -o obj/name_service.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/name_service/name_service.c

obj/sel_addrspace.o: trusted/service_runtime/sel_addrspace.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_addrspace.c

obj/sel_ldr.o: trusted/service_runtime/sel_ldr.c
	@gcc ${CCFLAGS} -o obj/sel_ldr.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_ldr.c

obj/sel_ldr-inl.o: trusted/service_runtime/sel_ldr-inl.c
	@gcc ${CCFLAGS} -o obj/sel_ldr-inl.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_ldr-inl.c

obj/sel_ldr_standard.o: trusted/service_runtime/sel_ldr_standard.c
	@gcc ${CCFLAGS} -o obj/sel_ldr_standard.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_ldr_standard.c

obj/sel_ldr_thread_interface.o: trusted/service_runtime/sel_ldr_thread_interface.c
	@gcc ${CCFLAGS} -o obj/sel_ldr_thread_interface.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_ldr_thread_interface.c

obj/sel_mem.o: trusted/service_runtime/sel_mem.c
	@gcc ${CCFLAGS} -o obj/sel_mem.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_mem.c

obj/sel_qualify.o: trusted/service_runtime/sel_qualify.c
	@gcc ${CCFLAGS} -o obj/sel_qualify.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_qualify.c

obj/sel_util-inl.o: trusted/service_runtime/sel_util-inl.c
	@gcc ${CCFLAGS} -o obj/sel_util-inl.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_util-inl.c

obj/sel_validate_image.o: trusted/service_runtime/sel_validate_image.c
	@gcc ${CCFLAGS} -o obj/sel_validate_image.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/sel_validate_image.c

obj/nacl_ldt_x86.o: trusted/service_runtime/arch/x86/nacl_ldt_x86.c
	@gcc ${CCFLAGS} -o obj/nacl_ldt_x86.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86/nacl_ldt_x86.c

obj/nacl_app_64.o: trusted/service_runtime/arch/x86_64/nacl_app_64.c
	@gcc ${CCFLAGS} -o obj/nacl_app_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/nacl_app_64.c

obj/nacl_switch_to_app_64.o: trusted/service_runtime/arch/x86_64/nacl_switch_to_app_64.c
	@gcc ${CCFLAGS} -o obj/nacl_switch_to_app_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/nacl_switch_to_app_64.c

obj/nacl_tls_64.o: trusted/service_runtime/arch/x86_64/nacl_tls_64.c
	@gcc ${CCFLAGS} -o obj/nacl_tls_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/nacl_tls_64.c

obj/sel_addrspace_x86_64.o: trusted/service_runtime/arch/x86_64/sel_addrspace_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace_x86_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/sel_addrspace_x86_64.c

obj/sel_ldr_x86_64.o: trusted/service_runtime/arch/x86_64/sel_ldr_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_ldr_x86_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/sel_ldr_x86_64.c

obj/sel_rt_64.o: trusted/service_runtime/arch/x86_64/sel_rt_64.c
	@gcc ${CCFLAGS} -o obj/sel_rt_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/sel_rt_64.c

obj/sel_addrspace_posix_x86_64.o: trusted/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c
	@gcc ${CCFLAGS} -o obj/sel_addrspace_posix_x86_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c

obj/sel_memory.o: trusted/service_runtime/linux/sel_memory.c
	@gcc ${CCFLAGS} -o obj/sel_memory.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/sel_memory.c

obj/nacl_oop_debugger_hooks.o: trusted/service_runtime/linux/nacl_oop_debugger_hooks.c
	@gcc ${CCFLAGS} -o obj/nacl_oop_debugger_hooks.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/nacl_oop_debugger_hooks.c

obj/nacl_thread_nice.o: trusted/service_runtime/linux/nacl_thread_nice.c
	@gcc ${CCFLAGS} -o obj/nacl_thread_nice.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/nacl_thread_nice.c

obj/nacl_ldt.o: trusted/service_runtime/linux/x86/nacl_ldt.c
	@gcc ${CCFLAGS} -o obj/nacl_ldt.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/x86/nacl_ldt.c

obj/sel_segments.o: trusted/service_runtime/linux/x86/sel_segments.c
	@gcc ${CCFLAGS} -o obj/sel_segments.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/x86/sel_segments.c

obj/nacl_signal.o: trusted/service_runtime/posix/nacl_signal.c
	@gcc ${CCFLAGS} -o obj/nacl_signal.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/posix/nacl_signal.c

obj/nacl_signal_64.o: trusted/service_runtime/linux/nacl_signal_64.c
	@gcc ${CCFLAGS} -o obj/nacl_signal_64.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/linux/nacl_signal_64.c

obj/env_cleanser.o: trusted/service_runtime/env_cleanser.c
	@gcc ${CCFLAGS} -o obj/env_cleanser.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/env_cleanser.c

obj/nacl_error_code.o: trusted/service_runtime/nacl_error_code.c
	@gcc ${CCFLAGS} -o obj/nacl_error_code.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/nacl_error_code.c

obj/manifest_proxy.o: trusted/manifest_name_service_proxy/manifest_proxy.c
	@gcc ${CCFLAGS} -o obj/manifest_proxy.o ${CCFLAGS0} ${CCFLAGS1} trusted/manifest_name_service_proxy/manifest_proxy.c

obj/nacl_simple_ltd_service.o: trusted/simple_service/nacl_simple_ltd_service.c
	@gcc ${CCFLAGS} -o obj/nacl_simple_ltd_service.o ${CCFLAGS0} ${CCFLAGS1} trusted/simple_service/nacl_simple_ltd_service.c

obj/nacl_simple_rservice.o: trusted/simple_service/nacl_simple_rservice.c
	@gcc ${CCFLAGS} -o obj/nacl_simple_rservice.o ${CCFLAGS0} ${CCFLAGS1} trusted/simple_service/nacl_simple_rservice.c

obj/nacl_simple_service.o: trusted/simple_service/nacl_simple_service.c
	@gcc ${CCFLAGS} -o obj/nacl_simple_service.o ${CCFLAGS0} ${CCFLAGS1} trusted/simple_service/nacl_simple_service.c

obj/nacl_thread_interface.o: trusted/threading/nacl_thread_interface.c
	@gcc ${CCFLAGS} -o obj/nacl_thread_interface.o ${CCFLAGS0} ${CCFLAGS1} trusted/threading/nacl_thread_interface.c

obj/gio_shm.o: trusted/gio/gio_shm.c
	@gcc ${CCFLAGS} -o obj/gio_shm.o ${CCFLAGS0} ${CCFLAGS1} trusted/gio/gio_shm.c

obj/gio_shm_unbounded.o: trusted/gio/gio_shm_unbounded.c
	@gcc ${CCFLAGS} -o obj/gio_shm_unbounded.o ${CCFLAGS0} ${CCFLAGS1} trusted/gio/gio_shm_unbounded.c

obj/gio_nacl_desc.o: trusted/gio/gio_nacl_desc.c
	@gcc ${CCFLAGS} -o obj/gio_nacl_desc.o ${CCFLAGS0} ${CCFLAGS1} trusted/gio/gio_nacl_desc.c

obj/nacl_desc_base.o: trusted/desc/nacl_desc_base.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_base.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_base.c

obj/nacl_desc_cond.o: trusted/desc/nacl_desc_cond.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_cond.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_cond.c

obj/nacl_desc_dir.o: trusted/desc/nacl_desc_dir.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_dir.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_dir.c

obj/nacl_desc_effector_trusted_mem.o: trusted/desc/nacl_desc_effector_trusted_mem.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_effector_trusted_mem.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_effector_trusted_mem.c

obj/nacl_desc_imc.o: trusted/desc/nacl_desc_imc.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_imc.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_imc.c

obj/nacl_desc_imc_shm.o: trusted/desc/nacl_desc_imc_shm.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_imc_shm.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_imc_shm.c

obj/nacl_desc_invalid.o: trusted/desc/nacl_desc_invalid.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_invalid.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_invalid.c

obj/nacl_desc_io.o: trusted/desc/nacl_desc_io.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_io.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_io.c

obj/nacl_desc_mutex.o: trusted/desc/nacl_desc_mutex.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_mutex.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_mutex.c

obj/nacl_desc_rng.o: trusted/desc/nacl_desc_rng.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_rng.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_rng.c

obj/nacl_desc_quota.o: trusted/desc/nacl_desc_quota.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_quota.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_quota.c

obj/nacl_desc_semaphore.o: trusted/desc/nacl_desc_semaphore.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_semaphore.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_semaphore.c

obj/nacl_desc_sync_socket.o: trusted/desc/nacl_desc_sync_socket.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_sync_socket.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nacl_desc_sync_socket.c

obj/nacl_pepper.o: trusted/desc/pepper/nacl_pepper.c
	@gcc ${CCFLAGS} -o obj/nacl_pepper.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/pepper/nacl_pepper.c

obj/nrd_all_modules.o: trusted/desc/nrd_all_modules.c
	@gcc ${CCFLAGS} -o obj/nrd_all_modules.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nrd_all_modules.c

obj/nrd_xfer_effector.o: trusted/desc/nrd_xfer_effector.c
	@gcc ${CCFLAGS} -o obj/nrd_xfer_effector.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/nrd_xfer_effector.c

obj/nacl_desc.o: trusted/desc/linux/nacl_desc.c
	@gcc ${CCFLAGS} -o obj/nacl_desc.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/linux/nacl_desc.c

obj/nacl_desc_sysv_shm.o:
	@gcc ${CCFLAGS} -o obj/nacl_desc_sysv_shm.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/linux/nacl_desc_sysv_shm.c

obj/nacl_desc_conn_cap.o: trusted/desc/linux/nacl_desc_sysv_shm.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_conn_cap.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/posix/nacl_desc_conn_cap.c

obj/nacl_desc_imc_bound_desc.o: trusted/desc/posix/nacl_desc_imc_bound_desc.c
	@gcc ${CCFLAGS} -o obj/nacl_desc_imc_bound_desc.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/posix/nacl_desc_imc_bound_desc.c

obj/nacl_makeboundsock.o: trusted/desc/posix/nacl_makeboundsock.c
	@gcc ${CCFLAGS} -o obj/nacl_makeboundsock.o ${CCFLAGS0} ${CCFLAGS1} trusted/desc/posix/nacl_makeboundsock.c

obj/nacl_perf_counter.o: trusted/perf_counter/nacl_perf_counter.c
	@gcc ${CCFLAGS} -o obj/nacl_perf_counter.o ${CCFLAGS0} ${CCFLAGS1} trusted/perf_counter/nacl_perf_counter.c

obj/nacl_refcount.o: trusted/nacl_base/nacl_refcount.c
	@gcc ${CCFLAGS} -o obj/nacl_refcount.o ${CCFLAGS0} ${CCFLAGS1} trusted/nacl_base/nacl_refcount.c

obj/container.o: trusted/service_runtime/generic_container/container.c
	@gcc ${CCFLAGS} -o obj/container.o ${CCFLAGS0} ${CCFLAGS1} trusted/service_runtime/generic_container/container.c

obj/fault_injection.o: trusted/fault_injection/fault_injection.c
	@gcc ${CCFLAGS} -o obj/fault_injection.o ${CCFLAGS0} ${CCFLAGS1} trusted/fault_injection/fault_injection.c

obj/nacl_os_qualify.o: trusted/platform_qualify/linux/nacl_os_qualify.c
	@gcc ${CCFLAGS} -o obj/nacl_os_qualify.o ${CCFLAGS0} ${CCFLAGS1} trusted/platform_qualify/linux/nacl_os_qualify.c

obj/sysv_shm_and_mmap.o: trusted/platform_qualify/linux/sysv_shm_and_mmap.c
	@gcc ${CCFLAGS} -o obj/sysv_shm_and_mmap.o ${CCFLAGS0} ${CCFLAGS1} trusted/platform_qualify/linux/sysv_shm_and_mmap.c

obj/nacl_dep_qualify.o: trusted/platform_qualify/posix/nacl_dep_qualify.c
	@gcc ${CCFLAGS} -o obj/nacl_dep_qualify.o ${CCFLAGS0} ${CCFLAGS1} trusted/platform_qualify/posix/nacl_dep_qualify.c

obj/nacl_cpuwhitelist.o: trusted/platform_qualify/arch/x86/nacl_cpuwhitelist.c
	@gcc ${CCFLAGS} -o obj/nacl_cpuwhitelist.o ${CCFLAGS0} ${CCFLAGS1} trusted/platform_qualify/arch/x86/nacl_cpuwhitelist.c

obj/nacl_dep_qualify_arch.o: trusted/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c
	@gcc ${CCFLAGS} -o obj/nacl_dep_qualify_arch.o ${CCFLAGS0} ${CCFLAGS1} trusted/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c

obj/ncvalidate_iter.o: trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.c
	@gcc ${CCFLAGS} -o obj/ncvalidate_iter.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter.c

obj/ncvalidate_iter_detailed.o: trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.c
	@gcc ${CCFLAGS} -o obj/ncvalidate_iter_detailed.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.c

obj/ncvalidator_registry.o: trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry.c
	@gcc ${CCFLAGS} -o obj/ncvalidator_registry.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry.c

obj/ncvalidator_registry_detailed.o: trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry_detailed.c
	@gcc ${CCFLAGS} -o obj/ncvalidator_registry_detailed.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncvalidator_registry_detailed.c

obj/nc_cpu_checks.o: trusted/validator/x86/ncval_reg_sfi/nc_cpu_checks.c
	@gcc ${CCFLAGS} -o obj/nc_cpu_checks.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_cpu_checks.c

obj/nc_illegal.o: trusted/validator/x86/ncval_reg_sfi/nc_illegal.c
	@gcc ${CCFLAGS} -o obj/nc_illegal.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_illegal.c

obj/nc_jumps.o: trusted/validator/x86/ncval_reg_sfi/nc_jumps.c
	@gcc ${CCFLAGS} -o obj/nc_jumps.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_jumps.c

obj/address_sets.o: trusted/validator/x86/ncval_reg_sfi/address_sets.c
	@gcc ${CCFLAGS} -o obj/address_sets.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/address_sets.c

obj/nc_jumps_detailed.o: trusted/validator/x86/ncval_reg_sfi/nc_jumps_detailed.c
	@gcc ${CCFLAGS} -o obj/nc_jumps_detailed.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_jumps_detailed.c

obj/nc_opcode_histogram.o: trusted/validator/x86/ncval_reg_sfi/nc_opcode_histogram.c
	@gcc ${CCFLAGS} -o obj/nc_opcode_histogram.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_opcode_histogram.c

obj/nc_protect_base.o: trusted/validator/x86/ncval_reg_sfi/nc_protect_base.c
	@gcc ${CCFLAGS} -o obj/nc_protect_base.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_protect_base.c

obj/nc_memory_protect.o: trusted/validator/x86/ncval_reg_sfi/nc_memory_protect.c
	@gcc ${CCFLAGS} -o obj/nc_memory_protect.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/nc_memory_protect.c

obj/ncvalidate_utils.o: trusted/validator/x86/ncval_reg_sfi/ncvalidate_utils.c
	@gcc ${CCFLAGS} -o obj/ncvalidate_utils.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncvalidate_utils.c

obj/ncval_decode_tables.o: trusted/validator/x86/ncval_reg_sfi/ncval_decode_tables.c
	@gcc ${CCFLAGS} -o obj/ncval_decode_tables.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncval_reg_sfi/ncval_decode_tables.c

obj/nccopycode.o: trusted/validator_x86/nccopycode.c
	@gcc ${CCFLAGS} -o obj/nccopycode.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator_x86/nccopycode.c

obj/nc_inst_iter.o: trusted/validator/x86/decoder/nc_inst_iter.c
	@gcc ${CCFLAGS} -o obj/nc_inst_iter.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/decoder/nc_inst_iter.c

obj/nc_inst_state.o: trusted/validator/x86/decoder/nc_inst_state.c
	@gcc ${CCFLAGS} -o obj/nc_inst_state.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/decoder/nc_inst_state.c

obj/nc_inst_trans.o: trusted/validator/x86/decoder/nc_inst_trans.c
	@gcc ${CCFLAGS} -o obj/nc_inst_trans.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/decoder/nc_inst_trans.c

obj/ncop_exps.o: trusted/validator/x86/decoder/ncop_exps.c
	@gcc ${CCFLAGS} -o obj/ncop_exps.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/decoder/ncop_exps.c

obj/ncopcode_desc.o: trusted/validator/x86/decoder/ncopcode_desc.c
	@gcc ${CCFLAGS} -o obj/ncopcode_desc.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/decoder/ncopcode_desc.c

obj/error_reporter.o: trusted/validator/x86/error_reporter.c
	@gcc ${CCFLAGS} -o obj/error_reporter.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/error_reporter.c

obj/halt_trim.o: trusted/validator/x86/halt_trim.c
	@gcc ${CCFLAGS} -o obj/halt_trim.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/halt_trim.c

obj/nacl_cpuid.o: trusted/validator/x86/nacl_cpuid.c
	@gcc ${CCFLAGS} -o obj/nacl_cpuid.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/nacl_cpuid.c

obj/ncinstbuffer.o: trusted/validator/x86/ncinstbuffer.c
	@gcc ${CCFLAGS} -o obj/ncinstbuffer.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/ncinstbuffer.c

obj/x86_insts.o: trusted/validator/x86/x86_insts.c
	@gcc ${CCFLAGS} -o obj/x86_insts.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/x86_insts.c

obj/nc_segment.o: trusted/validator/x86/nc_segment.c
	@gcc ${CCFLAGS} -o obj/nc_segment.o ${CCFLAGS0} ${CCFLAGS1} trusted/validator/x86/nc_segment.c

obj/nacl_exit.o: shared/platform/linux/nacl_exit.c
	@gcc ${CCFLAGS} -o obj/nacl_exit.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_exit.c

obj/nacl_find_addrsp.o: shared/platform/linux/nacl_find_addrsp.c
	@gcc ${CCFLAGS} -o obj/nacl_find_addrsp.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_find_addrsp.c

obj/nacl_host_desc.o: shared/platform/linux/nacl_host_desc.c
	@gcc ${CCFLAGS} -o obj/nacl_host_desc.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_host_desc.c

obj/nacl_host_dir.o: shared/platform/linux/nacl_host_dir.c
	@gcc ${CCFLAGS} -o obj/nacl_host_dir.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_host_dir.c

obj/nacl_secure_random.o: shared/platform/linux/nacl_secure_random.c
	@gcc ${CCFLAGS} -o obj/nacl_secure_random.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_secure_random.c

obj/nacl_semaphore.o: shared/platform/linux/nacl_semaphore.c
	@gcc ${CCFLAGS} -o obj/nacl_semaphore.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_semaphore.c

obj/nacl_thread_id.o: shared/platform/linux/nacl_thread_id.c
	@gcc ${CCFLAGS} -o obj/nacl_thread_id.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_thread_id.c

obj/nacl_threads.o: shared/platform/linux/nacl_threads.c
	@gcc ${CCFLAGS} -o obj/nacl_threads.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_threads.c

obj/nacl_time.o: shared/platform/linux/nacl_time.c
	@gcc ${CCFLAGS} -o obj/nacl_time.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_time.c

obj/nacl_timestamp.o: shared/platform/linux/nacl_timestamp.c
	@gcc ${CCFLAGS} -o obj/nacl_timestamp.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/nacl_timestamp.c

obj/condition_variable.o: shared/platform/linux/condition_variable.c
	@gcc ${CCFLAGS} -o obj/condition_variable.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/condition_variable.c

obj/lock.o: shared/platform/linux/lock.c
	@gcc ${CCFLAGS} -o obj/lock.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/linux/lock.c

obj/nacl_check.o: shared/platform/nacl_check.c
	@gcc ${CCFLAGS} -o obj/nacl_check.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_check.c

obj/nacl_global_secure_random.o: shared/platform/nacl_global_secure_random.c
	@gcc ${CCFLAGS} -o obj/nacl_global_secure_random.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_global_secure_random.c

obj/nacl_host_desc_common.o: shared/platform/nacl_host_desc_common.c
	@gcc ${CCFLAGS} -o obj/nacl_host_desc_common.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_host_desc_common.c

obj/nacl_interruptible_condvar.o: shared/platform/nacl_interruptible_condvar.c
	@gcc ${CCFLAGS} -o obj/nacl_interruptible_condvar.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_interruptible_condvar.c

obj/nacl_interruptible_mutex.o: shared/platform/nacl_interruptible_mutex.c
	@gcc ${CCFLAGS} -o obj/nacl_interruptible_mutex.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_interruptible_mutex.c

obj/nacl_log.o: shared/platform/nacl_log.c
	@gcc ${CCFLAGS} -o obj/nacl_log.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_log.c

obj/nacl_secure_random_common.o: shared/platform/nacl_secure_random_common.c
	@gcc ${CCFLAGS} -o obj/nacl_secure_random_common.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_secure_random_common.c

obj/nacl_sync_checked.o: shared/platform/nacl_sync_checked.c
	@gcc ${CCFLAGS} -o obj/nacl_sync_checked.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_sync_checked.c

obj/nacl_time_common.o: shared/platform/nacl_time_common.c
	@gcc ${CCFLAGS} -o obj/nacl_time_common.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/nacl_time_common.c

obj/platform_init.o: shared/platform/platform_init.c
	@gcc ${CCFLAGS} -o obj/platform_init.o ${CCFLAGS0} ${CCFLAGS1} shared/platform/platform_init.c

obj/gio.o: shared/gio/gio.c
	@gcc ${CCFLAGS} -o obj/gio.o ${CCFLAGS0} ${CCFLAGS1} shared/gio/gio.c

obj/gio_mem.o: shared/gio/gio_mem.c
	@gcc ${CCFLAGS} -o obj/gio_mem.o ${CCFLAGS0} ${CCFLAGS1} shared/gio/gio_mem.c

obj/gprintf.o: shared/gio/gprintf.c
	@gcc ${CCFLAGS} -o obj/gprintf.o ${CCFLAGS0} ${CCFLAGS1} shared/gio/gprintf.c

obj/gio_mem_snapshot.o: shared/gio/gio_mem_snapshot.c
	@gcc ${CCFLAGS} -o obj/gio_mem_snapshot.o ${CCFLAGS0} ${CCFLAGS1} shared/gio/gio_mem_snapshot.c

obj/invoke.o: shared/srpc/invoke.c
	@gcc ${CCFLAGS} -o obj/invoke.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/invoke.c

obj/module_init_fini.o: shared/srpc/module_init_fini.c
	@gcc ${CCFLAGS} -o obj/module_init_fini.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/module_init_fini.c

obj/nacl_srpc.o: shared/srpc/nacl_srpc.c
	@gcc ${CCFLAGS} -o obj/nacl_srpc.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/nacl_srpc.c

obj/nacl_srpc_message.o: shared/srpc/nacl_srpc_message.c
	@gcc ${CCFLAGS} -o obj/nacl_srpc_message.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/nacl_srpc_message.c

obj/rpc_log.o: shared/srpc/rpc_log.c
	@gcc ${CCFLAGS} -o obj/rpc_log.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/rpc_log.c

obj/rpc_serialize.o: shared/srpc/rpc_serialize.c
	@gcc ${CCFLAGS} -o obj/rpc_serialize.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/rpc_serialize.c

obj/rpc_service.o: shared/srpc/rpc_service.c
	@gcc ${CCFLAGS} -o obj/rpc_service.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/rpc_service.c

obj/rpc_server_loop.o: shared/srpc/rpc_server_loop.c
	@gcc ${CCFLAGS} -o obj/rpc_server_loop.o ${CCFLAGS0} ${CCFLAGS1} shared/srpc/rpc_server_loop.c

obj/abi.o: trusted/gdb_rsp/abi.cc
	@g++ ${CXXFLAGS} -o obj/abi.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/abi.cc

obj/host.o: trusted/gdb_rsp/host.cc
	@g++ ${CXXFLAGS} -o obj/host.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/host.cc

obj/packet.o: trusted/gdb_rsp/packet.cc
	@g++ ${CXXFLAGS} -o obj/packet.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/packet.cc

obj/session.o: trusted/gdb_rsp/session.cc
	@g++ ${CXXFLAGS} -o obj/session.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/session.cc

obj/target.o: trusted/gdb_rsp/target.cc
	@g++ ${CXXFLAGS} -o obj/target.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/target.cc

obj/util.o: trusted/gdb_rsp/util.cc
	@g++ ${CXXFLAGS} -o obj/util.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} -DGDB_RSP_ABI_X86 -DGDB_RSP_ABI_X86_64 trusted/gdb_rsp/util.cc

obj/nacl_desc_wrapper.o: trusted/desc/nacl_desc_wrapper.cc
	@g++ ${CXXFLAGS} -o obj/nacl_desc_wrapper.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/desc/nacl_desc_wrapper.cc

obj/nacl_imc_common.o: shared/imc/nacl_imc_common.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc_common.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} shared/imc/nacl_imc_common.cc

obj/nacl_imc_c.o: shared/imc/nacl_imc_c.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc_c.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} shared/imc/nacl_imc_c.cc

obj/nacl_imc_unistd.o: shared/imc/nacl_imc_unistd.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc_unistd.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} shared/imc/nacl_imc_unistd.cc

obj/nacl_imc.o: shared/imc/linux/nacl_imc.cc
	@g++ ${CXXFLAGS} -o obj/nacl_imc.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} shared/imc/linux/nacl_imc.cc

obj/refcount_base.o: shared/platform/refcount_base.cc
	@g++ ${CXXFLAGS} -o obj/refcount_base.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} shared/platform/refcount_base.cc

obj/nacl_debug.o: trusted/debug_stub/nacl_debug.cc
	@g++ ${CXXFLAGS} -o obj/nacl_debug.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/nacl_debug.cc

obj/debug_stub.o: trusted/debug_stub/debug_stub.cc
	@g++ ${CXXFLAGS} -o obj/debug_stub.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/debug_stub.cc

obj/event_common.o: trusted/debug_stub/event_common.cc
	@g++ ${CXXFLAGS} -o obj/event_common.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/event_common.cc

obj/platform_common.o: trusted/debug_stub/platform_common.cc
	@g++ ${CXXFLAGS} -o obj/platform_common.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/platform_common.cc

obj/transport_common.o: trusted/debug_stub/transport_common.cc
	@g++ ${CXXFLAGS} -o obj/transport_common.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/transport_common.cc

obj/debug_stub_posix.o: trusted/debug_stub/posix/debug_stub_posix.cc
	@g++ ${CXXFLAGS} -o obj/debug_stub_posix.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/posix/debug_stub_posix.cc

obj/mutex_impl.o: trusted/debug_stub/posix/mutex_impl.cc
	@g++ ${CXXFLAGS} -o obj/mutex_impl.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/posix/mutex_impl.cc

obj/platform_impl.o: trusted/debug_stub/posix/platform_impl.cc
	@g++ ${CXXFLAGS} -o obj/platform_impl.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/posix/platform_impl.cc

obj/thread_impl.o: trusted/debug_stub/posix/thread_impl.cc
	@g++ ${CXXFLAGS} -o obj/thread_impl.o ${CXXFLAGS1} ${CCFLAGS2} ${CCFLAGS4} trusted/debug_stub/posix/thread_impl.cc
