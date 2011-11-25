/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_THREADING_NACL_THREAD_INTERFACE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_THREADING_NACL_THREAD_INTERFACE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_threads.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"

EXTERN_C_BEGIN

struct NaClThreadInterface;
struct NaClThreadInterfaceVtbl;

typedef void *(*NaClThreadIfStartFunction)(struct NaClThreadInterface *tdb);

/*
 * Thread Factory.  Returns bool; used to get more threads.  In
 * object-oriented terms, this would be a "static virtual" except C++
 * doesn't support such.  It is considered okay for this to fail,
 * e.g., when shutdown is already in progress.  This interface is
 * intended to permit the following uses:
 *
 * (1) Keeping track of the number of threads running.  This is used
 * in the simple_service library so that (especially reverse services)
 * a plugin shutdown can wait for service threads to shutdown prior to
 * releasing shared resources.
 *
 * (2) In the service runtime, ensure that the VM lock is held prior
 * to spawning a new thread (and allocating a thread stack as a side
 * effect).  The LaunchCallback method can signal the thread launcher
 * that the thread stack allocation is complete and that the VM lock
 * may be dropped.  This is important for Address Space Squatting in
 * Windows to cover the temporary VM hole that can be created due to
 * mmap-like operations, and serves to ensure that trusted thread
 * stacks don't get allocated inside the untrusted address space.
 *
 * The newly constructed thread does not start until its StartThread
 * virtual function is invoked.  After Start is invoked, the ownership
 * of the reference returned by the factory is passed to the newly
 * started thread.
 */
typedef int (*NaClThreadIfFactoryFunction)(
    void                        *factory_data,
    NaClThreadIfStartFunction   fn_ptr,
    void                        *thread_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread);

struct NaClThreadInterface {
  struct NaClRefCount             base NACL_IS_REFCOUNT_SUBCLASS;
  NaClThreadIfFactoryFunction     factory;
  void                            *factory_data;
  size_t                          thread_stack_size;
  int                             thread_started;
  struct NaClThread               thread;
  NaClThreadIfStartFunction       fn_ptr;
  void                            *thread_data;
};

struct NaClThreadInterfaceVtbl {
  struct NaClRefCountVtbl       vbase;

  /* start thread may fail */
  int                           (*StartThread)(
      struct NaClThreadInterface *self);

  /*
   * Will be invoked in new thread prior to fn_ptr.
   */
  void                          (*LaunchCallback)(
      struct NaClThreadInterface *self);

  /*
   * Performs cleanup -- unref the NaClThreadInterface object (which
   * may in turn invoke the Dtor) and then invoke NaClThreadExit
   * without accessing any instance members.  Should not return.
   */
  void                          (*Exit)(
      struct NaClThreadInterface  *self,
      void                        *thread_return);
};

extern struct NaClThreadInterfaceVtbl const kNaClThreadInterfaceVtbl;

void NaClThreadInterfaceDtor(struct NaClRefCount *vself);

int NaClThreadInterfaceStartThread(struct NaClThreadInterface  *self);

void NaClThreadInterfaceLaunchCallback(struct NaClThreadInterface *self);

void NaClThreadInterfaceExit(struct NaClThreadInterface *self,
                             void                       *exit_code);

/*
 * The protected Ctor is intended for use by subclasses of
 * NaClThreadInterface.
 */
int NaClThreadInterfaceCtor_protected(
    struct NaClThreadInterface    *self,
    NaClThreadIfFactoryFunction   factory,
    void                          *factory_data,
    NaClThreadIfStartFunction     fn_ptr,
    void                          *thread_data,
    size_t                        thread_stack_size);

int NaClThreadInterfaceThreadFactory(
    void                        *factory_data,
    NaClThreadIfStartFunction   fn_ptr,
    void                        *thread_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread);

/*
 * A common use case is to use a factory to construct a thread and
 * launch it, so we provide a utility for doing so.
 */
int NaClThreadInterfaceConstructAndStartThread(
    NaClThreadIfFactoryFunction factory_fn,
    void                        *factory_data,
    NaClThreadIfStartFunction   thread_fn_ptr,
    void                        *thread_data,
    size_t                      thread_stack_size,
    struct NaClThreadInterface  **out_new_thread);

EXTERN_C_END

#endif
