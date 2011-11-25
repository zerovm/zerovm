/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "native_client/src/trusted/threading/nacl_thread_interface.h"

#include "native_client/src/include/nacl_compiler_annotations.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

void WINAPI NaClThreadInterfaceStart(void *data) {
  struct NaClThreadInterface  *tif =
      (struct NaClThreadInterface *) data;
  void                        *thread_return;

  NaClLog(4,
          ("Entered NaClThreadInterfaceStart: thread object 0x%"NACL_PRIxPTR
           " starting.\n"),
          (uintptr_t) tif);  /* NaClThreadId() implicitly printed */
  (*NACL_VTBL(NaClThreadInterface, tif)->LaunchCallback)(tif);
  thread_return = (*tif->fn_ptr)(tif);
  NaClLog(4,
          ("NaClThreadInterfaceStart: thread object 0x%"NACL_PRIxPTR
           " returned 0x%"NACL_PRIxPTR".\n"),
          (uintptr_t) tif,
          (uintptr_t) thread_return);  /* NaClThreadId() implicitly printed */
  (*NACL_VTBL(NaClThreadInterface, tif)->Exit)(tif, thread_return);
  NaClLog(LOG_FATAL,
          "NaClThreadInterface: Exit member function did not exit thread\n");
}

int NaClThreadInterfaceCtor_protected(
    struct NaClThreadInterface    *self,
    NaClThreadIfFactoryFunction   factory,
    void                          *factory_data,
    NaClThreadIfStartFunction     fn_ptr,
    void                          *thread_data,
    size_t                        thread_stack_size) {

  NaClLog(3, "Entered NaClThreadInterfaceThreadPlacementFactory\n");
  if (!NaClRefCountCtor((struct NaClRefCount *) self)) {
    NaClLog(3,
            "NaClThreadInterfaceThreadPlacementFactory,"
            " NaClRefCountCtor base class ctor failed\n");
    return 0;
  }

  self->factory = factory;
  self->factory_data = factory_data;
  self->thread_stack_size = thread_stack_size;
  self->fn_ptr = fn_ptr;
  self->thread_data = thread_data;
  self->thread_started = 0;
  NACL_VTBL(NaClRefCount, self) =
      (struct NaClRefCountVtbl const *) &kNaClThreadInterfaceVtbl;
  NaClLog(3,
          "Leaving NaClThreadInterfaceThreadPlacementFactory, returning 1\n");
  return 1;
}

int NaClThreadInterfaceThreadFactory(
    void                        *factory_data,
    NaClThreadIfStartFunction   fn_ptr,
    void                        *thread_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread) {
  struct NaClThreadInterface  *new_thread;
  int                         rv;

  NaClLog(3, "Entered NaClThreadInterfaceThreadFactory\n");
  new_thread = malloc(sizeof *new_thread);
  if (NULL == new_thread) {
    NaClLog(3, "NaClThreadInterfaceThreadFactory: no memory!\n");
    return 0;
  }
  if (0 != (rv =
            NaClThreadInterfaceCtor_protected(
                new_thread,
                NaClThreadInterfaceThreadFactory,
                factory_data,
                fn_ptr,
                thread_data,
                thread_stack_size))) {
    *out_new_thread = new_thread;
    NaClLog(3,
            "NaClThreadInterfaceThreadFactory: new thread object"
            " 0x%"NACL_PRIxPTR" (not started)\n",
            (uintptr_t) new_thread);
    new_thread = NULL;
  }
  free(new_thread);
  NaClLog(3,
          "Leaving NaClThreadInterfaceThreadFactory, returning %d\n",
          rv);
  return rv;
}

void NaClThreadInterfaceDtor(struct NaClRefCount *vself) {
  struct NaClThreadInterface *self =
      (struct NaClThreadInterface *) vself;
  CHECK(self->thread_started == 0);
  self->fn_ptr = NULL;
  self->thread_data = NULL;
  NACL_VTBL(NaClRefCount, self) = &kNaClRefCountVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

int NaClThreadInterfaceStartThread(struct NaClThreadInterface  *self) {
  int rv;

  NaClLog(3,
          "Entered NaClThreadInterfaceStartThread: self 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  CHECK(self->thread_started == 0);

  rv = NaClThreadCtor(&self->thread,
                      NaClThreadInterfaceStart,
                      self,
                      self->thread_stack_size);
  if (rv) {
    self->thread_started = 1;
  }
  NaClLog(3, "Leaving NaClThreadInterfaceStartThread, rv=%d\n", rv);
  return rv;
}

/*
 * Default LaunchCallback does nothing.  We could have made this "pure
 * virtual" by doing NaClLog(LOG_FATAL, ...) in the body (at least
 * detected at runtime).
 */
void NaClThreadInterfaceLaunchCallback(struct NaClThreadInterface *self) {
  NaClLog(3,
          "NaClThreadInterfaceLaunchCallback: thread 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
}

void NaClThreadInterfaceExit(struct NaClThreadInterface *self,
                             void                       *exit_code) {
  NaClLog(3,
          "NaClThreadInterfaceExit: thread 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) self);
  self->thread_started = 0;  /* on way out */
  NaClRefCountUnref((struct NaClRefCount *) self);
  /* only keep low order bits of the traditional void* return */
  NaClThreadExit((int) (uintptr_t) exit_code);
}

struct NaClThreadInterfaceVtbl const kNaClThreadInterfaceVtbl = {
  {
    NaClThreadInterfaceDtor,
  },
  NaClThreadInterfaceStartThread,
  NaClThreadInterfaceLaunchCallback,
  NaClThreadInterfaceExit,
};

int NaClThreadInterfaceConstructAndStartThread(
    NaClThreadIfFactoryFunction factory_fn,
    void                        *factory_data,
    NaClThreadIfStartFunction   thread_fn_ptr,
    void                        *thread_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread) {
  struct NaClThreadInterface  *new_thread;

  NaClLog(3,
          "NaClThreadInterfaceConstructAndStartThread: invoking factory"
          " function 0x%"NACL_PRIxPTR", factory data 0x%"NACL_PRIxPTR"\n",
          (uintptr_t) factory_fn, (uintptr_t) factory_data);
  if (!(*factory_fn)(factory_data,
                     thread_fn_ptr,
                     thread_data,
                     thread_stack_size,
                     &new_thread)) {
    NaClLog(3,
            ("NaClThreadInterfaceConstructAndStartThread:"
             " factory 0x%"NACL_PRIxPTR" failed to produce!\n"),
            (uintptr_t) factory_fn);
    new_thread = NULL;
    goto abort;
  }
  NaClLog(3,
          "NaClThreadInterfaceConstructAndStartThread: StartThread vfn\n");
  if (!(*NACL_VTBL(NaClThreadInterface, new_thread)->StartThread)(
          new_thread)) {
    NaClLog(3,
            ("NaClThreadInterfaceConstructAndStartThread:"
             " factory 0x%"NACL_PRIxPTR" produced a thread 0x%"NACL_PRIxPTR
             " that won't start!\n"),
            (uintptr_t) factory_fn,
            (uintptr_t) new_thread);
    NaClRefCountUnref((struct NaClRefCount *) new_thread);
    new_thread = NULL;
    goto abort;
  }
  NaClLog(4,
          ("NaClThreadInterfaceConstructAndStartThread: thread 0x%"NACL_PRIxPTR
           " started\n"),
          (uintptr_t) new_thread);
abort:
  *out_new_thread = new_thread;
  return new_thread != NULL;
}
