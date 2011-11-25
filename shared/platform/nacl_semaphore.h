/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl semaphore cross-platform abstraction
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SEMAPHORE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SEMAPHORE_H_

#include "include/nacl_base.h"

#include "shared/platform/nacl_sync.h"

#if NACL_WINDOWS
#include "shared/platform/win/nacl_semaphore.h"
#elif NACL_LINUX
#include "shared/platform/linux/nacl_semaphore.h"
#elif NACL_OSX
#include "shared/platform/osx/nacl_semaphore.h"
#else
#error "Unknown platform!!!"
#endif

#include "shared/platform/nacl_sync.h"

EXTERN_C_BEGIN

struct NaClSemaphore;

int NaClSemCtor(struct NaClSemaphore *sem, int32_t value);

void NaClSemDtor(struct NaClSemaphore *sem);

NaClSyncStatus NaClSemWait(struct NaClSemaphore *sem);

NaClSyncStatus NaClSemTryWait(struct NaClSemaphore *sem);

NaClSyncStatus NaClSemPost(struct NaClSemaphore *sem);

int NaClSemGetValue(struct NaClSemaphore *sem);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_SEMAPHORE_H_ */
