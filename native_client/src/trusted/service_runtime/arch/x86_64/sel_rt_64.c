/*
 * Copyright 2009 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_rt.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"


uintptr_t NaClGetThreadCtxSp(struct NaClThreadContext  *th_ctx) {
  return (uintptr_t) th_ctx->rsp;
}


void NaClSetThreadCtxSp(struct NaClThreadContext  *th_ctx, uintptr_t sp) {
  th_ctx->rsp = (nacl_reg_t) sp;
}

void NaClInitGlobals() {
  /* no need to save segment registers */
  ;
}

int NaClThreadContextCtor(struct NaClThreadContext  *ntcp,
                          struct NaClApp            *nap,
                          nacl_reg_t                prog_ctr,
                          nacl_reg_t                stack_ptr,
                          uint32_t                  tls_idx) {
  ntcp->rax = 0;
  ntcp->rbx = 0;
  ntcp->rcx = 0;
  ntcp->rdx = 0;

  ntcp->rbp = stack_ptr;  /* must be a valid stack addr! */
  ntcp->rsi = 0;
  ntcp->rdi = 0;
  ntcp->rsp = stack_ptr;

  ntcp->r8 = 0;
  ntcp->r9 = 0;
  ntcp->r10 = 0;
  ntcp->r11 = 0;

  ntcp->r12 = 0;
  ntcp->r13 = 0;
  ntcp->r14 = 0;
  ntcp->r15 = nap->mem_start;

  ntcp->prog_ctr = NaClUserToSys(nap, prog_ctr);
  ntcp->new_prog_ctr = 0;
  ntcp->sysret = -NACL_ABI_EINVAL;

  ntcp->tls_base = NULL;
  ntcp->tls_idx = tls_idx;

  return 1;
}
