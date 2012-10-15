/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime time abstraction layer.
 * This is the host-OS-dependent implementation.
 */

#include <errno.h>
#include "src/include/nacl_macros.h"
#include "src/platform/nacl_time.h"
#include "src/platform/linux/nacl_time_types.h"
#include "src/service_runtime/sel_ldr.h"

#define NANOS_PER_UNIT  (1000*1000*1000)

/*
 * Syscall return value mapper.  The linux raw syscall convention is
 * that anything positive and anything smaller than a certain negative
 * number are valid return values (think addresses in the 2GB range,
 * since user addresses may be in the first 3 GB, and the kernel lives
 * in the 4th GB), and anything else (smallish negative numbers) are
 * errors from Linux (possibly OSX).  In such a case, the syscall
 * wrapper will take the negative value and store it into the
 * thread-specific errno variable, and return -1 instead.  Since we
 * are using these wrappers, we merely detect when any host OS syscall
 * returned -1, and pass -errno back to the NaCl app.  (The syscall
 * wrappers on the NaCl app side will similarly follow the
 * negative-values-are-errors convention).
 */
static INLINE intptr_t NaClXlateSysRet(intptr_t rv) {
  return (rv != -1) ? rv : -NaClXlateErrno(errno);
}

static struct NaClTimeState gNaClTimeState;

void NaClTimeInternalInit(struct NaClTimeState *ntsp) {
  ntsp->time_resolution_ns = NACL_NANOS_PER_MICRO;
}

void NaClTimeInternalFini(struct NaClTimeState *ntsp) {
  UNREFERENCED_PARAMETER(ntsp);
}

void NaClTimeInit(void) {
  NaClTimeInternalInit(&gNaClTimeState);
}

void NaClTimeFini(void) {
  NaClTimeInternalFini(&gNaClTimeState);
}

int NaClGetTimeOfDay(struct nacl_abi_timeval *tv) {
  struct timeval  sys_tv;
  int             retval;

  retval = gettimeofday(&sys_tv, NULL);
  if (0 == retval) {
    tv->nacl_abi_tv_sec = sys_tv.tv_sec;
    tv->nacl_abi_tv_usec = sys_tv.tv_usec;
  }

  retval = NaClXlateSysRet(retval);
  return retval;
}

