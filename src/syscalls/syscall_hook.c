/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NaCl service run-time.
 */
#include <errno.h>
#include "src/syscalls/switch_to_app.h"
#include "src/main/nacl_globals.h"
#include "src/loader/sel_rt.h"
#include "src/main/manifest_setup.h"
#include "src/syscalls/trap.h"

/* serve trap invoked from the untrusted code */
NORETURN void NaClSyscallCSegHook()
{
  struct NaClApp *nap;
  struct NaClThreadContext *user;
  uintptr_t tramp_ret;
  nacl_reg_t user_ret;
  size_t sysnum;
  uintptr_t sp_user;
  uintptr_t sp_sys;

  /* restore trusted side environment */
  nap = gnap; /* restore NaClApp object */
  user = nacl_user;
  sp_user = NaClGetThreadCtxSp(user);
  sp_sys = sp_user;

  /*
   * sp_sys points to the top of user stack where there is a retaddr to
   * trampoline slot
   */
  tramp_ret = *(uintptr_t *)sp_sys;
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

  /* fail if nacl syscall received */
  ZLOGFAIL(sysnum != 0, EINVAL, "nacl syscalls not supported (#%d received)", sysnum);

  /*
   * syscall_args must point to the first argument of a system call.
   * System call arguments are placed on the untrusted user stack.
   */
  nap->sysret = TrapHandler(nap, *(uint32_t*)sp_sys);

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
