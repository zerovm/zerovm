/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
#ifndef SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__
#define SERVICE_RUNTIME_NACL_SYSCALL_HANDLERS_H__ 1

#include "src/service_runtime/include/machine/_types.h"

struct NaClSyscallTableEntry {
  int32_t (*handler)(struct NaClApp *nap);
};

/* these are defined in the platform specific code */
extern struct NaClSyscallTableEntry nacl_syscall[];

void NaClSyscallTableInit();

/* common syscalls/functions */
int32_t NaClCommonSysMmapIntern(struct NaClApp        *nap,
                                void                  *start,
                                size_t                length,
                                int                   prot,
                                int                   flags,
                                int                   d,
                                nacl_abi_off_t        offset);

EXTERN_C_END

#endif
