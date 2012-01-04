/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl kernel / service run-time system call numbers
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_SYSCALLS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_INCLUDE_BITS_NACL_SYSCALLS_H_

/* intentionally not using zero */

#define Trap                             0 /* "one ring" syscall. future replacement for other syscalls */
#define NACL_sys_sysbrk                 20
#define NACL_sys_mmap                   21 /* to remove */
#define NACL_sys_munmap                 22 /* to remove */
#define NACL_sys_exit                   30
#define NACL_sys_mutex_create           70
#define NACL_sys_tls_init               82
#define NACL_sys_tls_get                84

#define NACL_MAX_SYSCALLS               110

#endif
