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
 * restore trusted context and call trap()
 */
#include "src/syscalls/switch_to_app.h"
#include "src/syscalls/trap.h"

/* serve trap invoked from the untrusted code */
NORETURN void SyscallHook()
{
  struct NaClApp *nap;
  uintptr_t tramp_ret;
  size_t sysnum;
  uintptr_t sp_user;
  uintptr_t sp_sys;

  /* restore trusted side environment */
  nap = gnap; /* restore NaClApp object */
  sp_user = GetThreadCtxSp(nacl_user);
  sp_sys = sp_user;

  /*
   * sp_sys points to the top of user stack where there is a retaddr to
   * trampoline slot
   */
  tramp_ret = *(uintptr_t*)sp_sys;
  sysnum = (tramp_ret - (MEM_START + NACL_SYSCALL_START_ADDR))
      >> NACL_SYSCALL_BLOCK_SHIFT;

  /*
   * getting user return address (the address where we need to return after
   * system call) from the user stack. (see stack layout above)
   */
  nacl_user->prog_ctr = *(uintptr_t*)(sp_sys + NACL_USERRET_FIX);

  /*
   * Fix the user stack, throw away return addresses from the top of the stack.
   * After this fix, the first argument to a system call must be on the top of
   * the user stack (see user stack layout above)
   */
  sp_sys += NACL_SYSARGS_FIX;
  sp_user += NACL_SYSCALLRET_FIX;
  SetThreadCtxSp(nacl_user, sp_user);

  /* fail if nacl syscall received */
  ZLOGFAIL(sysnum != 0, EINVAL, "nacl syscall #%d received", sysnum);

  /*
   * syscall_args must point to the first argument of a system call.
   * System call arguments are placed on the untrusted user stack.
   */
  nacl_user->sysret = TrapHandler(nap, *(uint32_t*)sp_sys);

  /*
   * before switching back to user module, we need to make sure that the
   * user_ret is properly sandboxed.
   */
  nacl_user->prog_ctr = NaClSandboxCodeAddr(nacl_user->prog_ctr);

  /* give control to the user side */
  ContextSwitch(nacl_user);
  ZLOGFAIL(1, EFAULT, "the unreachable has been reached");
}
