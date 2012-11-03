/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time.
 */
#include <errno.h>
#include "src/service_runtime/nacl_switch_to_app.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/nacl_syscalls.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/sel_rt_64.h"
#include "src/manifest/manifest_setup.h"

#ifdef DISABLE_RDTSC
#include <signal.h>
#include <inttypes.h>
#include <sys/prctl.h>
#include <linux/prctl.h>
#endif

/*
 * d'b: make syscall invoked from the untrusted code
 */
NORETURN void NaClSyscallCSegHook()
{
  struct NaClApp *nap;
  struct NaClThreadContext *user;
  uintptr_t tramp_ret;
  nacl_reg_t user_ret;
  size_t sysnum;
  uintptr_t sp_user;
  uintptr_t sp_sys;

  /* d'b: nexe just invoked some syscall. increase syscalls counter */
  nap = gnap; /* restore NaClApp object */
  user = nacl_user; /* restore from global */
  sp_user = NaClGetThreadCtxSp(user);
  sp_sys = NaClUserToSysStackAddr(nap, sp_user);

#ifdef DISABLE_RDTSC
  /* prevent rdtsc execution */
  ZLOGFAIL(prctl(PR_SET_TSC, PR_TSC_ENABLE) == -1,
      errno, "cannot allow rdtsc execution");
#endif

  /*
   * sp_sys points to the top of user stack where there is a retaddr to
   * trampoline slot
   */
  tramp_ret = *(uintptr_t *)sp_sys;
  tramp_ret = NaClUserToSysStackAddr(nap, tramp_ret);
  sysnum = (tramp_ret - (nap->mem_start + NACL_SYSCALL_START_ADDR)) >> NACL_SYSCALL_BLOCK_SHIFT;

  /*
   * getting user return address (the address where we need to return after
   * system call) from the user stack. (see stack layout above)
   */
  user_ret = *(uintptr_t *)(sp_sys + NACL_USERRET_FIX);

  /*
   * Fix the user stack, throw away return addresses from the top of the stack.
   * After this fix, the first argument to a system call must be on the top of
   * the user stack (see user stack layout above)
   */
  sp_sys += NACL_SYSARGS_FIX;
  sp_user += NACL_SYSCALLRET_FIX;
  NaClSetThreadCtxSp(user, sp_user);

  /* debug print to log */
  ZLOGS(LOG_INSANE, "system call number %ld", sysnum);

  if(sysnum >= NACL_MAX_SYSCALLS)
  {
    ZLOG(LOG_ERROR, "INVALID system call %ld", sysnum);
    nap->sysret = -EINVAL;
  }
  else
  {
    /*
     * syscall_args is used by Decoder functions in
     * nacl_syscall_handlers.c which is automatically generated file
     * and placed in the
     * scons-out/.../gen/src/service_runtime/
     * directory.  syscall_args must point to the first argument of a
     * system call. System call arguments are placed on the untrusted
     * user stack.
     */
    nap->syscall_args = (uintptr_t *)sp_sys;
    nap->sysret = (*(nap->syscall_table[sysnum].handler))(nap);
  }

  /*
   * before switching back to user module, we need to make sure that the
   * user_ret is properly sandboxed.
   */
  user_ret = (nacl_reg_t)NaClSandboxCodeAddr(nap, (uintptr_t)user_ret);

  /* d'b: give control to the nexe */
  NaClSwitchToApp(nap, user_ret);

  /* NOTREACHED */
  ZLOGFAIL(1, EFAULT, "NORETURN NaClSwitchToApp returned!?!");
}
