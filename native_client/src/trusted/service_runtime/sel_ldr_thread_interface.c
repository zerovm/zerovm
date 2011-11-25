/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/service_runtime/sel_ldr_thread_interface.h"

#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/threading/nacl_thread_interface.h"

int NaClAddrSpSquattingThreadIfFactoryFunction(
    void                        *factory_data,
    NaClThreadIfStartFunction   thread_fn_ptr,
    void                        *thread_fn_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread);  /* fwd */

int NaClAddrSpSquattingThreadIfCtor_protected(
    struct NaClAddrSpSquattingThreadInterface *self,
    void                                      *factory_data,
    NaClThreadIfStartFunction                 thread_fn_ptr,
    void                                      *thread_fn_data,
    size_t                                    thread_stack_size) {

  NaClLog(3, "Entered NaClAddrSpSquattingThreadIfCtor_protected\n");
  if (!NaClThreadInterfaceCtor_protected(
          (struct NaClThreadInterface *) self,
          NaClAddrSpSquattingThreadIfFactoryFunction,
          factory_data,  /* nap */
          thread_fn_ptr,
          thread_fn_data,
          thread_stack_size)) {
    NaClLog(3,
            ("NaClAddrSpSquattingThreadIfCtor_protected: placement base class"
             " ctor failed\n"));
    return 0;
  }

  self->nap = (struct NaClApp *) factory_data;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl *) &kNaClAddrSpSquattingThreadInterfaceVtbl;

  NaClLog(3, "Leaving NaClAddrSpSquattingThreadIfCtor_protected\n");
  return 1;
}

int NaClAddrSpSquattingThreadIfFactoryFunction(
    void                        *factory_data,
    NaClThreadIfStartFunction   thread_fn_ptr,
    void                        *thread_fn_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread) {
  struct NaClAddrSpSquattingThreadInterface *new_thread;
  int                                       rv = 0;

  NaClLog(3, "Entered NaClAddrSpSquattingThreadIfFactoryFunction\n");
  new_thread = (struct NaClAddrSpSquattingThreadInterface *)
      malloc(sizeof *new_thread);
  if (NULL == new_thread) {
    rv = 0;
    goto cleanup;
  }

  if (!(rv =
        NaClAddrSpSquattingThreadIfCtor_protected(new_thread,
                                                  factory_data,
                                                  thread_fn_ptr,
                                                  thread_fn_data,
                                                  thread_stack_size))) {
    goto cleanup;
  }

  *out_new_thread = (struct NaClThreadInterface *) new_thread;
  new_thread = NULL;

cleanup:
  free(new_thread);
  NaClLog(3,
          "Leaving NaClAddrSpSquattingThreadIfFactoryFunction, rv %d\n",
          rv);
  return rv;
}

int NaClAddrSpSquattingThreadIfStartThread(
    struct NaClThreadInterface *vself) {
  struct NaClAddrSpSquattingThreadInterface *self =
      (struct NaClAddrSpSquattingThreadInterface *) vself;
  NaClLog(2,
          "NaClAddrSpSquattingThreadIfStartThread: waiting to start thread\n");
  NaClVmHoleWaitToStartThread(self->nap);

  if (!NaClThreadInterfaceStartThread(vself)) {
    NaClLog(LOG_ERROR,
            "NaClAddrSpSquattingThreadIfStartThread: could not start thread\n");
    NaClVmHoleThreadStackIsSafe(self->nap);
    return 0;
  }
  NaClLog(2,
          "NaClAddrSpSquattingThreadIfStartThread: thread started\n");
  return 1;
}

void NaClAddrSpSquattingThreadIfLaunchCallback(
    struct NaClThreadInterface *vself) {
  struct NaClAddrSpSquattingThreadInterface *self =
      (struct NaClAddrSpSquattingThreadInterface *) vself;

  NaClVmHoleThreadStackIsSafe(self->nap);
}

struct NaClThreadInterfaceVtbl const kNaClAddrSpSquattingThreadInterfaceVtbl = {
  {
    NaClThreadInterfaceDtor,
  },
  NaClAddrSpSquattingThreadIfStartThread,
  NaClAddrSpSquattingThreadIfLaunchCallback,
  NaClThreadInterfaceExit,
};
