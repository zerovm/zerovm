/*
 * Copyright 2008  The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl service runtime syscall inline header file.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_LINUX_NACL_SYSCALL_INL_H_
#define NATIVE_CLIENT_SERVICE_RUNTIME_LINUX_NACL_SYSCALL_INL_H_

#include "src/platform/nacl_host_dir.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/sel_ldr.h"

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

#endif
