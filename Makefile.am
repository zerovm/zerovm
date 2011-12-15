#AM_CXXFLAGS = -D__STDC_LIMIT_MACROS=1 -D__STDC_FORMAT_MACROS=1
noinst_LIBRARIES = libenv_cleanser.a \
	libgio.a \
	libgio_wrapped_desc.a \
	libimc.a \
	libnacl_base.a \
	libnacl_error_code.a \
	libnacl_fault_inject.a \
	libnacl_perf_counter.a \
	libnc_decoder_x86_64.a \
	libnc_opcode_modeling_x86_64.a \
	libnccopy_x86_64.a \
	libncval_base_x86_64.a \
	libncval_reg_sfi_x86_64.a \
	libncvalidate_x86_64.a \
	libnrd_xfer.a \
	libplatform.a \
	libplatform_qual_lib.a \
	libthread_interface.a \
	libsel.a \
	libmanifest.a
libenv_cleanser_a_SOURCES = src/service_runtime/env_cleanser.c
libgio_a_SOURCES = src/gio/gio.c \
	src/gio/gio_mem.c \
	src/gio/gprintf.c \
	src/gio/gio_mem_snapshot.c
libgio_wrapped_desc_a_SOURCES = src/gio/gio_shm.c \
	src/gio/gio_shm_unbounded.c \
	src/gio/gio_nacl_desc.c
libimc_a_SOURCES = src/imc/linux/nacl_imc.cc \
	src/imc/nacl_imc_unistd.cc \
	src/imc/nacl_imc_c.cc \
	src/imc/nacl_imc_common.cc
libnacl_base_a_SOURCES = src/nacl_base/nacl_refcount.c
libnacl_error_code_a_SOURCES = src/service_runtime/nacl_error_code.c
libnacl_fault_inject_a_SOURCES = src/fault_injection/fault_injection.c
libnacl_perf_counter_a_SOURCES = src/perf_counter/nacl_perf_counter.c
libnc_decoder_x86_64_a_SOURCES = src/validator/x86/decoder/nc_inst_iter.c \
	src/validator/x86/decoder/nc_inst_state.c \
	src/validator/x86/decoder/nc_inst_trans.c \
	src/validator/x86/decoder/ncop_exps.c
libnc_opcode_modeling_x86_64_a_SOURCES = src/validator/x86/decoder/ncopcode_desc.c
libnccopy_x86_64_a_SOURCES = src/validator_x86/nccopycode_stores.S \
	src/validator_x86/nccopycode.c
libncval_base_x86_64_a_SOURCES = src/validator/x86/error_reporter.c \
	src/validator/x86/halt_trim.c \
	src/validator/x86/nacl_cpuid.c \
	src/validator/x86/nacl_xgetbv.S \
	src/validator/x86/ncinstbuffer.c \
	src/validator/x86/x86_insts.c \
	src/validator/x86/nc_segment.c
libncval_reg_sfi_x86_64_a_SOURCES = src/validator/x86/ncval_reg_sfi/ncvalidate_iter.c \
	src/validator/x86/ncval_reg_sfi/ncvalidate_iter_detailed.c \
	src/validator/x86/ncval_reg_sfi/ncvalidator_registry.c \
	src/validator/x86/ncval_reg_sfi/ncvalidator_registry_detailed.c \
	src/validator/x86/ncval_reg_sfi/nc_cpu_checks.c \
	src/validator/x86/ncval_reg_sfi/nc_illegal.c \
	src/validator/x86/ncval_reg_sfi/nc_jumps.c \
	src/validator/x86/ncval_reg_sfi/address_sets.c \
	src/validator/x86/ncval_reg_sfi/nc_jumps_detailed.c \
	src/validator/x86/ncval_reg_sfi/nc_opcode_histogram.c \
	src/validator/x86/ncval_reg_sfi/nc_protect_base.c \
	src/validator/x86/ncval_reg_sfi/nc_memory_protect.c \
	src/validator/x86/ncval_reg_sfi/ncvalidate_utils.c \
	src/validator/x86/ncval_reg_sfi/ncval_decode_tables.c
libncvalidate_x86_64_a_SOURCES = src/validator/x86/64/ncvalidate.c
libnrd_xfer_a_SOURCES = src/desc/nacl_desc_base.c \
	src/desc/nacl_desc_cond.c \
	src/desc/nacl_desc_dir.c \
	src/desc/nacl_desc_effector_trusted_mem.c \
	src/desc/nacl_desc_imc.c \
	src/desc/nacl_desc_imc_shm.c \
	src/desc/nacl_desc_invalid.c \
	src/desc/nacl_desc_io.c \
	src/desc/nacl_desc_mutex.c \
	src/desc/nacl_desc_rng.c \
	src/desc/nacl_desc_semaphore.c \
	src/desc/nacl_desc_sync_socket.c \
	src/desc/nrd_all_modules.c \
	src/desc/nrd_xfer.c \
	src/desc/nrd_xfer_effector.c \
	src/desc/nacl_desc_wrapper.cc \
	src/desc/linux/nacl_desc.c \
	src/desc/linux/nacl_desc_sysv_shm.c \
	src/desc/posix/nacl_desc_conn_cap.c \
	src/desc/posix/nacl_desc_imc_bound_desc.c \
	src/desc/posix/nacl_makeboundsock.c
libplatform_a_SOURCES = src/platform/linux/nacl_exit.c \
	src/platform/linux/nacl_find_addrsp.c \
	src/platform/linux/nacl_host_desc.c \
	src/platform/linux/nacl_host_dir.c \
	src/platform/linux/nacl_secure_random.c \
	src/platform/linux/nacl_semaphore.c \
	src/platform/linux/nacl_thread_id.c \
	src/platform/linux/nacl_threads.c \
	src/platform/linux/nacl_time.c \
	src/platform/linux/nacl_timestamp.c \
	src/platform/linux/condition_variable.c \
	src/platform/linux/lock.c \
	src/platform/nacl_check.c \
	src/platform/nacl_global_secure_random.c \
	src/platform/nacl_host_desc_common.c \
	src/platform/nacl_interruptible_condvar.c \
	src/platform/nacl_interruptible_mutex.c \
	src/platform/nacl_log.c \
	src/platform/nacl_secure_random_common.c \
	src/platform/nacl_sync_checked.c \
	src/platform/nacl_time_common.c \
	src/platform/platform_init.c \
	src/platform/refcount_base.cc
libplatform_qual_lib_a_SOURCES = src/platform_qualify/linux/nacl_os_qualify.c \
	src/platform_qualify/linux/sysv_shm_and_mmap.c \
	src/platform_qualify/posix/nacl_dep_qualify.c \
	src/platform_qualify/arch/x86/nacl_cpuwhitelist.c \
	src/platform_qualify/arch/x86_64/nacl_dep_qualify_arch.c
libthread_interface_a_SOURCES = src/threading/nacl_thread_interface.c
libsel_a_SOURCES = src/service_runtime/dyn_array.c \
	src/service_runtime/elf_util.c \
	src/service_runtime/nacl_all_modules.c \
	src/service_runtime/nacl_app_thread.c \
	src/service_runtime/nacl_desc_effector_ldr.c \
	src/service_runtime/nacl_globals.c \
	src/service_runtime/nacl_memory_object.c \
	src/service_runtime/nacl_signal_common.c \
	src/service_runtime/nacl_stack_safety.c \
	src/service_runtime/nacl_syscall_common.c \
	src/service_runtime/nacl_syscall_handlers.c \
	src/service_runtime/nacl_syscall_hook.c \
	src/service_runtime/nacl_text.c \
	src/service_runtime/sel_addrspace.c \
	src/service_runtime/sel_ldr.c \
	src/service_runtime/sel_ldr-inl.c \
	src/service_runtime/sel_ldr_standard.c \
	src/service_runtime/sel_ldr_thread_interface.c \
	src/service_runtime/sel_mem.c \
	src/service_runtime/sel_qualify.c \
	src/service_runtime/sel_util-inl.c \
	src/service_runtime/sel_validate_image.c \
	src/service_runtime/arch/x86/nacl_ldt_x86.c \
	src/service_runtime/arch/x86_64/nacl_app_64.c \
	src/service_runtime/arch/x86_64/nacl_switch_64.S \
	src/service_runtime/arch/x86_64/nacl_switch_to_app_64.c \
	src/service_runtime/arch/x86_64/nacl_syscall_64.S \
	src/service_runtime/arch/x86_64/nacl_tls_64.c \
	src/service_runtime/arch/x86_64/sel_addrspace_x86_64.c \
	src/service_runtime/arch/x86_64/sel_ldr_x86_64.c \
	src/service_runtime/arch/x86_64/sel_rt_64.c \
	src/service_runtime/arch/x86_64/tramp_64.S \
	src/service_runtime/arch/x86_64/sel_addrspace_posix_x86_64.c \
	src/service_runtime/linux/sel_memory.c \
	src/service_runtime/linux/nacl_thread_nice.c \
	src/service_runtime/linux/x86/nacl_ldt.c \
	src/service_runtime/linux/x86/sel_segments.c \
	src/service_runtime/posix/nacl_signal.c \
	src/service_runtime/linux/nacl_signal_64.c 
libmanifest_a_SOURCES = src/manifest/manifest_parser.c \
	src/manifest/manifest_setup.c \
	src/manifest/mount_channel.c \
	src/manifest/prefetch.c \
	src/manifest/preload.c \
	src/manifest/premap.c \
	src/manifest/trap.c \
	src/manifest/zmq_syscalls.c \
	src/manifest/md5.c
bin_PROGRAMS = zerovm
zerovm_LDFLAGS = -lstdc++ $(AM_LDFLAGS)
zerovm_SOURCES = src/service_runtime/sel_main.c
zerovm_LDADD =  libsel.a \
	libenv_cleanser.a \
	libnacl_error_code.a \
        libthread_interface.a \
	libgio_wrapped_desc.a \
        libnrd_xfer.a \
        libnacl_perf_counter.a \
        libnacl_base.a \
        libimc.a \
        libnacl_fault_inject.a \
        libplatform.a \
        libplatform_qual_lib.a \
        libncvalidate_x86_64.a \
        libncval_reg_sfi_x86_64.a \
        libnccopy_x86_64.a \
        libnc_decoder_x86_64.a \
        libnc_opcode_modeling_x86_64.a \
        libncval_base_x86_64.a \
        libplatform.a \
        libgio.a \
	libmanifest.a
check_PROGRAMS = x86_validator_tests_halt_trim \
	service_runtime_tests \
	x86_decoder_tests_nc_inst_state \
	x86_validator_tests_nc_inst_bytes \
	x86_validator_tests_nc_remaining_memory 
#	manifest_parser_test 
x86_validator_tests_halt_trim_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
x86_validator_tests_halt_trim_SOURCES = src/validator/x86/halt_trim_tests.cc
x86_validator_tests_halt_trim_LDADD = libncvalidate_x86_64.a \
	libncval_reg_sfi_x86_64.a \
	libnccopy_x86_64.a \
	libnc_decoder_x86_64.a \
	libnc_opcode_modeling_x86_64.a \
	libncval_base_x86_64.a \
	libplatform.a \
	libgio.a \
	$(top_srcdir)/gtest/libgtest.a
service_runtime_tests_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
service_runtime_tests_SOURCES = src/service_runtime/sel_ldr_test.cc src/service_runtime/sel_mem_test.cc src/service_runtime/sel_memory_unittest.cc src/service_runtime/unittest_main.cc
service_runtime_tests_LDADD = libsel.a libenv_cleanser.a libthread_interface.a libgio_wrapped_desc.a libnrd_xfer.a libnacl_perf_counter.a libnacl_base.a libimc.a libnacl_fault_inject.a libplatform.a libncvalidate_x86_64.a libncval_reg_sfi_x86_64.a libnccopy_x86_64.a libnc_decoder_x86_64.a libnc_opcode_modeling_x86_64.a libncval_base_x86_64.a libplatform.a libgio.a $(top_srcdir)/gtest/libgtest.a libmanifest.a
x86_decoder_tests_nc_inst_state_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
x86_decoder_tests_nc_inst_state_SOURCES = src/validator/x86/decoder/nc_inst_state_tests.cc src/validator/x86/ncval_reg_sfi/ncval_decode_tables.c 
x86_decoder_tests_nc_inst_state_LDADD = libnccopy_x86_64.a libnc_decoder_x86_64.a libnc_opcode_modeling_x86_64.a libncval_base_x86_64.a libplatform.a libgio.a $(top_srcdir)/gtest/libgtest.a
x86_validator_tests_nc_inst_bytes_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
x86_validator_tests_nc_inst_bytes_SOURCES = src/validator/x86/nc_inst_bytes_tests.cc
x86_validator_tests_nc_inst_bytes_LDADD = libncvalidate_x86_64.a libncval_reg_sfi_x86_64.a libnccopy_x86_64.a libnc_decoder_x86_64.a libnc_opcode_modeling_x86_64.a libncval_base_x86_64.a libplatform.a libgio.a $(top_srcdir)/gtest/libgtest.a
x86_validator_tests_nc_remaining_memory_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
x86_validator_tests_nc_remaining_memory_SOURCES = src/validator/x86/nc_remaining_memory_tests.cc
x86_validator_tests_nc_remaining_memory_LDADD = libncvalidate_x86_64.a libncval_reg_sfi_x86_64.a libnccopy_x86_64.a libnc_decoder_x86_64.a libnc_opcode_modeling_x86_64.a libncval_base_x86_64.a libplatform.a libgio.a $(top_srcdir)/gtest/libgtest.a
manifest_parser_test_CXXFLAGS = -DNACL_TRUSTED_BUT_NOT_TCB=1 $(AM_CXXFLAGS)
manifest_parser_test_SOURCES = src/manifest/manifest_parser_test.cc
manifest_parser_test_LDADD = libsel.a libenv_cleanser.a libthread_interface.a libgio_wrapped_desc.a libnrd_xfer.a libnacl_perf_counter.a libnacl_base.a libimc.a libnacl_fault_inject.a libplatform.a libncvalidate_x86_64.a libncval_reg_sfi_x86_64.a libnccopy_x86_64.a libnc_decoder_x86_64.a libnc_opcode_modeling_x86_64.a libncval_base_x86_64.a libplatform.a libgio.a $(top_srcdir)/gtest/libgtest.a libmanifest.a
TESTS = $(check_PROGRAMS)
