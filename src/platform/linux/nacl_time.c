/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Server Runtime time abstraction layer.
 * This is the host-OS-dependent implementation.
 */

#include "include/nacl_macros.h"
#include "src/platform/nacl_time.h"
#include "src/platform/linux/nacl_time_types.h"
#include "src/service_runtime/linux/nacl_syscall_inl.h"

#define NANOS_PER_UNIT  (1000*1000*1000)

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

