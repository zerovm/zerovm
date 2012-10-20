/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime, C-level context switch code.
 */
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_switch_to_app.h"

#define NORETURN_PTR NORETURN

static NORETURN_PTR void (*NaClSwitch)(struct NaClThreadContext *context);

/*
 * d'b: a new cpu detection routine. just differ nosse/sse/avx
 * returns -1 if cpu has no sse, 0 for sse and 1 for avx spu
 */
static int CPUTest()
{
  int r[4] = {0};

  __asm("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(1), "c"(0));
  if((r[3] & (1 << 25)) == 0) return -1;
  if((r[2] & (1 << 28)) == 0) return 0;
  return 1;
}

void NaClInitSwitchToApp(struct NaClApp *nap)
{
  int cpu = CPUTest();

  UNREFERENCED_PARAMETER(nap);
  ZLOGFAIL(cpu == -1, EFAULT, "zerovm needs SSE CPU");
  NaClSwitch = cpu == 0 ? NaClSwitchSSE : NaClSwitchAVX;
  ZLOGS(LOG_DEBUG, "%s cpu detected", cpu == 0 ? "SSE" : "AVX");
}

/*
 * switch to the nacl module (untrusted content)
 */
NORETURN void NaClSwitchToApp(struct NaClApp *nap, nacl_reg_t new_prog_ctr)
{
  nacl_user->new_prog_ctr = new_prog_ctr;
  nacl_user->sysret = nap->sysret;

  /* d'b(todo): remove one of flags below */
  nap->user_side_flag = 0; /* remove "user side call" mark */
  nap->trusted_code = 0; /* we are going to the untrusted code */

  NaClSwitch(nacl_user);
}
