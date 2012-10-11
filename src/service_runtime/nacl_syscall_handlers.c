/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* todo(d'b): revise the headers */
#include "include/nacl_platform.h"
#include "src/desc/nacl_desc_io.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/include/bits/nacl_syscalls.h"
#include "src/service_runtime/include/sys/stat.h"
#include "src/manifest/trap.h"
#include "include/nacl_assert.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/include/sys/errno.h" /* d'b */

struct NaClSyscallTableEntry nacl_syscall[NACL_MAX_SYSCALLS] = {{0}};

static int32_t NotImplementedDecoder(struct NaClApp *nap)
{
  UNREFERENCED_PARAMETER(nap);
  return -NACL_ABI_ENOSYS;
}

static void NaClAddSyscall(int num, int32_t(*fn)(struct NaClApp *))
{
  if(nacl_syscall[num].handler != &NotImplementedDecoder)
  {
    NaClLog(LOG_FATAL, "Duplicate syscall number %d\n", num);
  }
  nacl_syscall[num].handler = fn;
}

/*
 * syscalls main routines.
 * note that we only need 1 syscall trap() other syscalls are only used
 * for nexe prolog in a future all syscalls except trap can be removed.
 * update: nacl mmap, munmap and most part of sysbrk rip
 */
int32_t NaClSysSysbrk(struct NaClApp *nap, uintptr_t new_break)
{
  /* todo(d'b): find appropriate macro (range check) or write it */
  if(new_break >= nap->data_end && new_break < nap->heap_end)
    nap->break_addr = new_break;
  return (int32_t)nap->break_addr;
}

/* d'b: duplicate of trap() exit. remove it when trap() replace syscalls */
int32_t NaClSysExit(struct NaClApp *nap, int status)
{
  NaClLog(1, "Exit syscall handler: %d\n", status);
  nap->system_manifest->user_ret_code = status;
  longjmp(user_exit, status);

  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

/* tls create */
#define CLEANUP(cond, code) if(cond) {retval = code; goto cleanup;}
int32_t NaClSysTls_Init(struct NaClApp *nap, void *thread_ptr)
{
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sys_tls;

  NaClLog(3, ("Entered NaClCommonSysTls_Init(0x%08"NACL_PRIxPTR
      ", 0x%08"NACL_PRIxPTR")\n"), (uintptr_t) nap, (uintptr_t) thread_ptr);

  /* Verify that the address in the app's range and translated from
   * nacl module address to service runtime address - a nop on ARM
   */
  sys_tls = NaClUserToSysAddrRange(nap, (uintptr_t) thread_ptr, 4);
  NaClLog(4, "NaClCommonSysTls_Init: thread_ptr 0x%p, sys_tls 0x%"NACL_PRIxPTR"\n",
          thread_ptr, sys_tls);

  CLEANUP(kNaClBadAddress == sys_tls, -NACL_ABI_EFAULT);
  nap->sys_tls = sys_tls;
  retval = 0;

cleanup:
  return retval;
}
#undef CLEANUP

/* tls get */
int32_t NaClSysTls_Get(struct NaClApp *nap) {
  uint32_t user_tls;

  /* too frequently used, and syscall-number level logging suffices */
  user_tls = (int32_t) NaClSysToUser(nap, nap->sys_tls);
  return user_tls;
}

/*
 * decoders. get call parameters from the user stack and call the syscall
 */

/* this function was automagically generated */
static int32_t NaClSysSysbrkDecoder(struct NaClApp *nap) {
  struct NaClSysSysbrkArgs {
    uint32_t new_break;
  } p = *(struct NaClSysSysbrkArgs *) nap->syscall_args;

  return NaClSysSysbrk(nap, (uintptr_t) p.new_break);
}

/* this function was automagically generated */
static int32_t NaClSysExitDecoder(struct NaClApp *nap) {
  struct NaClSysExitArgs {
    uint32_t status;
  } p = *(struct NaClSysExitArgs *) nap->syscall_args;

  return NaClSysExit(nap, (int)  p.status);
}

/* this function was automagically generated */
static int32_t NaClSysTls_InitDecoder(struct NaClApp *nap)
{
    struct NaClSysTls_InitArgs {
    uint32_t thread_ptr;
  } p = *(struct NaClSysTls_InitArgs *) nap->syscall_args;

  return NaClSysTls_Init(nap, (void *) (uintptr_t) p.thread_ptr);
}

/* this function was automagically generated */
static int32_t NaClSysTls_GetDecoder(struct NaClApp *nap)
{
  return NaClSysTls_Get(nap);
}

/*
 * only needed by nexe prologue. if user need real functionality
 * zrt library can be used (in the future)
 */
static int32_t NaClSysMutex_CreateDecoder(struct NaClApp *nap)
{
  return 1;
}

/* d'b: trap(). see documentation for the details */
static int32_t TrapDecoder(struct NaClApp *nap)
{
  struct TrapDecoderArgs
  {
    uint32_t args;
  } p = *(struct TrapDecoderArgs*) nap->syscall_args;

  return TrapHandler(nap, p.args);
}

/*
 * d'b: see documentation about "one ring" tonneling syscall
 * note: all syscalls except trap() only need for nexe prolog
 */
void NaClSyscallTableInit() {
  int i;
  for (i = 0; i < NACL_MAX_SYSCALLS; ++i) {
     nacl_syscall[i].handler = &NotImplementedDecoder;
  }

  NaClAddSyscall(Trap, &TrapDecoder); /* 0. added onering call */
  NaClAddSyscall(NACL_sys_sysbrk, &NaClSysSysbrkDecoder); /* 20 */
  NaClAddSyscall(NACL_sys_exit, &NaClSysExitDecoder); /* 30 */
  NaClAddSyscall(NACL_sys_tls_init, &NaClSysTls_InitDecoder); /* 82 */
  NaClAddSyscall(NACL_sys_tls_get, &NaClSysTls_GetDecoder); /* 84 */
  NaClAddSyscall(NACL_sys_mutex_create, &NaClSysMutex_CreateDecoder); /* 70 */
}
