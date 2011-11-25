#!/bin/sh
#SHOULD BE RUN FROM THE "SANDBOX ROOT FOLDER"
#--------------------------------------------

#temporary! "-Llibs2kill" option added to avoid linker error "ld: cannot find library"
#all sandbox libs are moved to one folder: "libs2kill"
#how to make ld to take libs from original locations?
#g++-4.4 -o native_client/scons-out/opt-linux-x86-64/obj/src/trusted/service_runtime/sel_ldr -Wl,-z,noexecstack -m64 -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=/home/bigcrunch/sandbox_hack/native_client/scons-out/opt-linux-x86-64/lib native_client/scons-out/opt-linux-x86-64/obj/src/trusted/service_runtime/sel_main.o -L/usr/lib -Lscons-out/opt-linux-x86-64/lib -L/home/bigcrunch/sandbox_hack/ppapi/native_client/scons-out/opt-linux-x86-64/lib -lsel -lenv_cleanser -lnacl_error_code -lmanifest_proxy -lsimple_service -lthread_interface -lgio_wrapped_desc -lnonnacl_srpc -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lcontainer -lnacl_fault_inject -lplatform -lplatform_qual_lib -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -ldebug_stub_init -lgdb_rsp -ldebug_stub -lrt -lpthread -lcrypto -ldl -Llibs2kill
g++-4.4 -o sel_ldr -Wl,-z,noexecstack -m64 -L/usr/lib64 -pie -Wl,-z,relro -Wl,-z,now -Wl,-rpath=native_client/scons-out/opt-linux-x86-64/lib native_client/scons-out/opt-linux-x86-64/obj/src/trusted/service_runtime/sel_main.o -L/usr/lib -Lnative_client/scons-out/opt-linux-x86-64/lib -lsel -lenv_cleanser -lnacl_error_code -lmanifest_proxy -lsimple_service -lthread_interface -lgio_wrapped_desc -lnonnacl_srpc -lnrd_xfer -lnacl_perf_counter -lnacl_base -limc -lcontainer -lnacl_fault_inject -lplatform -lplatform_qual_lib -lncvalidate_x86_64 -lncval_reg_sfi_x86_64 -lnccopy_x86_64 -lnc_decoder_x86_64 -lnc_opcode_modeling_x86_64 -lncval_base_x86_64 -lplatform -lgio -ldebug_stub_init -lgdb_rsp -ldebug_stub -lrt -lpthread -lcrypto -ldl -Llib



# stop here
#--------------------------------------------
exit

g++-4.4
-o
native_client/scons-out/opt-linux-x86-64/obj/src/trusted/service_runtime/sel_ldr
-Wl,-z,noexecstack
-m64
-L/usr/lib64
-pie
-Wl,-z,relro
-Wl,-z,now
-Wl,-rpath=/home/bigcrunch/sandbox_hack/native_client/scons-out/opt-linux-x86-64/lib
native_client/scons-out/opt-linux-x86-64/obj/src/trusted/service_runtime/sel_main.o
-L/usr/lib
-Lnative_client/scons-out/opt-linux-x86-64/lib
-lsel
-lenv_cleanser
-lnacl_error_code
-lmanifest_proxy
-lsimple_service
-lthread_interface
-lgio_wrapped_desc
-lnonnacl_srpc
-lnrd_xfer
-lnacl_perf_counter
-lnacl_base
-limc
-lcontainer
-lnacl_fault_inject
-lplatform
-lplatform_qual_lib
-lncvalidate_x86_64
-lncval_reg_sfi_x86_64
-lnccopy_x86_64
-lnc_decoder_x86_64
-lnc_opcode_modeling_x86_64
-lncval_base_x86_64
-lplatform
-lgio
-ldebug_stub_init
-lgdb_rsp
-ldebug_stub
-lrt
-lpthread
-lcrypto
-ldl
-Llib
