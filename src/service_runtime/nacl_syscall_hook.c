/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time.
 */

#include "include/portability.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "src/platform/nacl_exit.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/nacl_switch_to_app.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_rt.h"

#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/bits/nacl_syscalls.h"
#include "src/service_runtime/nacl_app_thread.h"
#include "src/service_runtime/nacl_stack_safety.h"
#include "src/service_runtime/nacl_globals.h" /* d'b */
#include "src/manifest/trap.h" /* d'b: ResumeCpuClock(), PauseCpuClock() */
#include "src/manifest/manifest_setup.h" /* d'b: ResumeCpuClock(), PauseCpuClock() */

/*
 * ###
 * d'b:
 * all changes are temporary. just to test a new design w/o threads
 */
NORETURN void NaClSyscallCSegHook()
{
  struct NaClApp            *nap;
  struct NaClThreadContext  *user;
  uintptr_t                 tramp_ret;
  nacl_reg_t                user_ret;
  size_t                    sysnum;
  uintptr_t                 sp_user;
  uintptr_t                 sp_sys;

  /*
   * d'b: nexe just invoked some syscall. stop cpu time counting
   * increase syscalls counter (correction for setup call will be
   * corrected later). small mallocs and other calls which are
   * not really "system" will be accounted anyway!
   */
  nap = gnap; /* restore NaClApp object */
  PauseCpuClock(nap);
  AccountingSyscallsInc(nap);
  user = nacl_user; /* restore from global */
  sp_user = NaClGetThreadCtxSp(user);

  /*
   * on x86_32 user stack:
   *  esp+0:  retaddr from lcall
   *  esp+4:  code seg from lcall
   *  esp+8:  retaddr to user module
   *  esp+c:  ...
   *
   * on ARM user stack
   *   sp+0:  retaddr from trampoline slot
   *   sp+4:  retaddr to user module
   *   sp+8:  arg0 to system call
   *   sp+c:  arg1 to system call
   *   sp+10: ....
   */
  sp_sys = NaClUserToSysStackAddr(nap, sp_user);

  /*
   * sp_sys points to the top of user stack where there is a retaddr to
   * trampoline slot
   */
  tramp_ret = *(uintptr_t *)sp_sys;
  tramp_ret = NaClUserToSysStackAddr(nap, tramp_ret);

  sysnum = (tramp_ret - (nap->mem_start + NACL_SYSCALL_START_ADDR))
      >> NACL_SYSCALL_BLOCK_SHIFT;

  /*
   * getting user return address (the address where we need to return after
   * system call) from the user stack. (see stack layout above)
   */
  user_ret = *(uintptr_t *) (sp_sys + NACL_USERRET_FIX);

  /*
   * Fix the user stack, throw away return addresses from the top of the stack.
   * After this fix, the first argument to a system call must be on the top of
   * the user stack (see user stack layout above)
   */
  sp_sys += NACL_SYSARGS_FIX;
  sp_user += NACL_SYSCALLRET_FIX;
  NaClSetThreadCtxSp(user, sp_user);

  if (sysnum >= NACL_MAX_SYSCALLS) {
    NaClLog(2, "INVALID system call %"NACL_PRIdS"\n", sysnum);
    nap->sysret = -NACL_ABI_EINVAL;
  } else {
    /*
     * syscall_args is used by Decoder functions in
     * nacl_syscall_handlers.c which is automatically generated file
     * and placed in the
     * scons-out/.../gen/src/service_runtime/
     * directory.  syscall_args must point to the first argument of a
     * system call. System call arguments are placed on the untrusted
     * user stack.
     */
    nap->syscall_args = (uintptr_t *) sp_sys;
    nap->sysret = (*(nap->syscall_table[sysnum].handler))(nap); //###
  }

  /*
   * before switching back to user module, we need to make sure that the
   * user_ret is properly sandboxed.
   */
  user_ret = (nacl_reg_t) NaClSandboxCodeAddr(nap, (uintptr_t)user_ret);
  NaClStackSafetyNowOnUntrustedStack();

  /* d'b: give control to the nexe. start cpu time counting */
  ResumeCpuClock(nap);
  NaClSwitchToApp(nap, user_ret);
  /* NOTREACHED */

  fprintf(stderr, "NORETURN NaClSwitchToApp returned!?!\n");
  NaClAbort();
}
