/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "native_client/src/untrusted/nacl/syscall_bindings_trampoline.h"

#if !defined(__pnacl__) && (defined(__x86_64__) || defined(__i386__))

typedef struct { char b[10]; } st_reg;
static st_reg st_zero;

/*
 * Not every x86-32 CPU supports SSE registers, but we're willing
 * to assume that machines we run this test on do.
 */
typedef struct { uint64_t b[2]; } xmm_reg __attribute__((aligned(16)));
static const xmm_reg xmm_zero;

static void infoleak_clear_state(void) {
  __asm__ volatile("fninit; fstpt %0" : "=m" (st_zero));
  __asm__ volatile("movaps %0, %%xmm7" :: "m" (xmm_zero));
}

__attribute__((noinline)) static int infoleak_check_state(void) {
  int ok = 1;
  st_reg st0;
  xmm_reg xmm7;
  __asm__("fstpt %0" : "=m" (st0));
  __asm__("movaps %%xmm7, %0" : "=m" (xmm7));
  if (memcmp(&st0, &st_zero, sizeof(st0)) != 0) {
    printf("x87 state leaked information!\n");
    ok = 0;
  }
  if (memcmp(&xmm7, &xmm_zero, sizeof(xmm7)) != 0) {
    printf("SSE state leaked information!\n");
    ok = 0;
  }
  return ok;
}

#define EXPECT_OK 1

#else

static void infoleak_clear_state(void) {
}

static int infoleak_check_state(void) {
  return 1;
}

#define EXPECT_OK 1

#endif

int main(void) {
  int result;
  int ok;

  infoleak_clear_state();

  result = NACL_SYSCALL(test_infoleak)();
  if (result != -ENOSYS) {
    printf("test_infoleak syscall returned %d\n", result);
    return 1;
  }

  ok = infoleak_check_state();

  return ok == EXPECT_OK ? 0 : 1;
}
