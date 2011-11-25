/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_THREAD_INTERFACE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_SEL_LDR_THREAD_INTERFACE_H_

/*
 * This is the NaClThreadInterface subclass that should be used by all
 * thread creators in the service runtime.  This interface will grab
 * the NaClApp's VM lock prior to actually creating the thread (and as
 * a side effect allocating the thread stack), so that no VM hole
 * exists in the untrusted address space.
 *
 * This is not needed except for in Windows, since on Linux and OSX we
 * use mmap and that maps over existing memory pages without creating
 * a temporary address space hole.  As an optimization, this code
 * might not actually grab the VM lock anywhere *except* on windows.
 */

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/threading/nacl_thread_interface.h"

EXTERN_C_BEGIN

struct NaClApp;

struct NaClAddrSpSquattingThreadInterface {
  struct NaClThreadInterface  base NACL_IS_REFCOUNT_SUBCLASS;
  struct NaClApp              *nap;
};

/* fwd */ extern
struct NaClThreadInterfaceVtbl const kNaClAddrSpSquattingThreadInterfaceVtbl;

int NaClAddrSpSquattingThreadIfFactoryFunction(
    void                        *factory_data,
    NaClThreadIfStartFunction   thread_fn_ptr,
    void                        *thread_fn_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread);

int NaClAddrSpSquattingThreadIfStartThread(
    struct NaClThreadInterface *vself);

void NaClAddrSpSquattingThreadIfLaunchCallback(
    struct NaClThreadInterface *vself);

EXTERN_C_END

#endif
