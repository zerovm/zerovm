/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time.
 */

#include "native_client/src/include/portability.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"
#include "native_client/src/trusted/service_runtime/nacl_switch_to_app.h"
#include "native_client/src/trusted/service_runtime/nacl_syscall_handlers.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/bits/nacl_syscalls.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_stack_safety.h"

int NaClArtificialDelay = -1;


/*
 * The first syscall is from the NaCl module's main thread, and there
 * are no other user threads yet, so NACLDELAY check and the NACLCLOCK
 * usages are okay; when the NaCl module is multithreaded, the
 * variables they initialize are read-only.
 */


void NaClMicroSleep(int microseconds) {
  static int    initialized = 0;
  static tick_t cpu_clock = 0;
  tick_t        now;
  tick_t        end;

  if (!initialized) {
    char *env = getenv("NACLCLOCK");
    if (NULL != env) {
      cpu_clock = strtoul(env, (char **) NULL, 0);
    }

    initialized = 1;
  }

  now = get_ticks();
  end = now + (cpu_clock * microseconds) / 1000000;
  NaClLog(5, "Now %"NACL_PRId64".  Waiting until %"NACL_PRId64".\n", now, end);
  while (get_ticks() < end)
    ;
}

NORETURN void NaClSyscallCSegHook(int32_t tls_idx) {
  struct NaClAppThread      *natp = nacl_thread[tls_idx];
  struct NaClApp            *nap = natp->nap;
  struct NaClThreadContext  *user = &natp->user;
  uintptr_t                 tramp_ret;
  nacl_reg_t                user_ret;
  size_t                    sysnum;
  uintptr_t                 sp_user;
  uintptr_t                 sp_sys;

  sp_user = NaClGetThreadCtxSp(user);

  /* sp must be okay for control to have gotten here */
#if !BENCHMARK
  NaClLog(4, "Entered NaClSyscallCSegHook\n");
  NaClLog(4, "user sp %"NACL_PRIxPTR"\n", sp_user);
#endif

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

#if !BENCHMARK
  NaClLog(4, "system call %"NACL_PRIuS"\n", sysnum);
#endif

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

  NaClStackSafetyNowOnTrustedStack();
  if (sysnum >= NACL_MAX_SYSCALLS) {
    NaClLog(2, "INVALID system call %"NACL_PRIdS"\n", sysnum);
    natp->sysret = -NACL_ABI_EINVAL;
  } else {
#if !BENCHMARK
    NaClLog(4, "making system call %"NACL_PRIdS", "
            "handler 0x%08"NACL_PRIxPTR"\n",
            sysnum, (uintptr_t) nap->syscall_table[sysnum].handler);
#endif
    /*
     * syscall_args is used by Decoder functions in
     * nacl_syscall_handlers.c which is automatically generated file
     * and placed in the
     * scons-out/.../gen/native_client/src/trusted/service_runtime/
     * directory.  syscall_args must point to the first argument of a
     * system call. System call arguments are placed on the untrusted
     * user stack.
     */
    natp->syscall_args = (uintptr_t *) sp_sys;
    natp->sysret = (*(nap->syscall_table[sysnum].handler))(natp);
  }
#if !BENCHMARK
  NaClLog(4,
          ("returning from system call %"NACL_PRIdS", return value %"NACL_PRId32
           " (0x%"NACL_PRIx32")\n"),
          sysnum, natp->sysret, natp->sysret);

  NaClLog(4, "return target 0x%08"NACL_PRIxNACL_REG"\n", user_ret);
  NaClLog(4, "user sp %"NACL_PRIxPTR"\n", sp_user);
#endif
  if (-1 == NaClArtificialDelay) {
    char *delay = getenv("NACLDELAY");
    if (NULL != delay) {
      NaClArtificialDelay = strtol(delay, (char **) NULL, 0);
      NaClLog(0, "ARTIFICIAL DELAY %d us\n", NaClArtificialDelay);
    } else {
      NaClArtificialDelay = 0;
    }
  }
  if (0 != NaClArtificialDelay) {
    NaClMicroSleep(NaClArtificialDelay);
  }

  /*
   * before switching back to user module, we need to make sure that the
   * user_ret is properly sandboxed.
   */
  user_ret = (nacl_reg_t) NaClSandboxCodeAddr(nap, (uintptr_t)user_ret);
  NaClStackSafetyNowOnUntrustedStack();

  NaClSwitchToApp(natp, user_ret);
  /* NOTREACHED */

  fprintf(stderr, "NORETURN NaClSwitchToApp returned!?!\n");
  NaClAbort();
}
