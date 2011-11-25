********************************************
  the project has been reorganized
  now this file should be moved to history
********************************************


attention!

this sandbox version is under construction
i am trying to remove from the project as much files as possible

also i am going to reorganize folders structure

notes:
-----------------------------------------------------------------------------------
i should carefully inspect validator, perhaps it has extra code regarding platforms other than linux/x86/64
later comment: we decided to keep the validator untouched. it would better to keep it google/community managed.


the future sandbox' look:
-----------------------------------------------------------------------------------
api
gtest
include
obj [folder to hold temporary content]
src
samples
test [folder to hold temporary content]

src will hold both "shared" and "trusted". when needless sandbox components will be removed, the folder will consist of:
gio
platform (it would be nice to rename it to something else since we only support 1 platform)
desc
fault injection [tiny folder]
nacl_base [tiny folder]
perf_counter [tiny folder]
service_runtime [huge folder] (perhaps it would be better to split this folder and name it different)
threading [tiny folder] (some files from service_runtime should be put here)
  nacl_app_thread
  nacl_desc_effector_loader
  
validator
validator_x86 [tiny folder] (should be joined with "validator" folder since we have only 1 platform)

once again:
----------
src/
  platform/
  desc/
  fault injection//
  nacl_base//
  perf_counter//
  
  loader\  
  manifest\
  memory_manager\
  
  service_runtime [huge folder] (perhaps it would be better to split this folder and name it different)
  threading//
  validator
  
later comment: done

progress:
-----------------------------------------------------------------------------------
  trusted/handle_pass/ldr_handle.h -- removed with its folder "handle_pass"
    trusted/service_runtime/sel_ldr.c -- fixed w/o notes "d'b"
    trusted/service_runtime/nacl_all_modules.c -- fixed as above
    
  trusted/desc/pepper/nacl_pepper.c --removed
  trusted/desc/pepper/nacl_pepper.h -- removed
    trusted/desc/nacl_desc_quota.c -- removed
    trusted/desc/nacl_desc_quota.h -- removed
      trusted/desc/nacl_desc_base.c -- fixed
      trusted/desc/nacl_desc_base.h -- fixed
      trusted/service_runtime/sel_ldr.c -- fixed
      
  trusted/service_runtime/win/exception_patch/ntdll_patch.h - removed  
  trusted/service_runtime/generic_container/container.c - removed
  trusted/service_runtime/generic_container/container.h - removed
  
  include/win/mman.h -- removed
    trusted/service_runtime/sel_ldr_standard.c -- fixed
    trusted/service_runtime/manifest_parser.c -- fixed
    trusted/service_runtime/sel_memory_unittest.cc -- fixed
    include/nacl_platform.h - fixed
    include/portability.h - fixed
    include/atomic_ops.h - fixed
    
  trusted/debug_stub/ -- removed
  service_runtime/nacl_debug_init.h --removed
  service_runtime/nacl_oop_debugger_hooks.h --removed
  service_runtime/linux/nacl_oop_debugger_hooks.c -- removed
    service_runtime/nacl_app_thread.c - fixed
    service_runtime/sel_ldr_standard.c - fixed
    service_runtime/nacl_all_modules.c - fixed
    
  trusted/gdb_rsp/ -- removed
  
  BIG ONE: srpc
  -------------    
  shared/srpc/ -- removed
    trusted/service_runtime/sel_ldr.c -- fixed
    trusted/service_runtime/sel_ldr.h -- fixed
    trusted/service_runtime/sel_main.c --  fixed
    trusted/service_runtime/nacl_all_modules.c -- fixed
    trusted/service_runtime/nacl_kern_services.c -- removed
      trusted/service_runtime/sel_ldr.h -- fixed
    trusted/service_runtime/env_cleanser.c -- 
    trusted/service_runtime/sel_ldr_standard.c -- 
    trusted/manifest_name_service_proxy -- removed
      trusted/service_runtime/sel_ldr.c -- 
      trusted/service_runtime/name_service/ -- removed
        trusted/service_runtime/sel_ldr.c -- fixed
    ...
    ... removed a lot of file and even folders
    ... made a lot of changes to code
    
    TODO:
    conspect all changes
    test zerovm as full as possible
    
  valgrind support has been removed together with srpc.
  
  restructure:
  -------------
  content of "shared" and "trusted" folders has been moved to newly created "src" (see above)  


candidates for removal/fix:
-----------------------------------------------------------------------------------  
  shared/utils/
  include/build/build_config.h
  include/base/ (or for massive refactoring)
  include/atomic_ops.h + include/nacl/atomic_ops.h + include/linux/x86/atomic_ops.h (should be joined)
  shared/gio + trusted/gio (should be joined)
  trusted/platform_qualify/ (we laready know everything about platform: linux x86 64)
  trusted/port/ (folder contain headers for each specific supported platform and we only support one of them)
  
  springboard and outersandbox:
  ----------------------------- 2 headers + changes to other files (especially to sel_main.c)
