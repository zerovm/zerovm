/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_NAME_SERVICE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_NAME_SERVICE_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"

#include "native_client/src/trusted/service_runtime/include/sys/nacl_name_service.h"
/* get NACL_NAME_SERVICE_CONNECTION_MAX */

#include "native_client/src/trusted/simple_service/nacl_simple_service.h"
#include "native_client/src/trusted/simple_service/nacl_simple_ltd_service.h"

#include "native_client/src/trusted/threading/nacl_thread_interface.h"

EXTERN_C_BEGIN

/*
 * NaClStartNameService should only be called once, and also serves
 * as a module initializer.
 *
 * This function creates a bound socket / socket address pair, and
 * spawns a thread that accepts connections on the bound socket and
 * services requests on the resultant connections.  A maximum number
 * of connections is defined, since well-behaving programs should not
 * need more than one (absent excessive opaque layering/abstractions)
 * name service lookup connection.
 *
 * The socket address NaClDescConnCap is returned, to be entered into
 * the NaClApp's open file table at a well-known location.
 */

struct NaClNameServiceEntry;  /* fwd */

struct NaClNameService {
  struct NaClSimpleLtdService base NACL_IS_REFCOUNT_SUBCLASS;

  struct NaClMutex            mu;
  /*
   * |mu| protects the service entries hanging off of |head|.
   */
  struct NaClNameServiceEntry *head;
};

int NaClNameServiceCtor(struct NaClNameService      *self,
                        NaClThreadIfFactoryFunction thread_factory_fn,
                        void                        *thread_factory_data);

int NaClNameServiceCreateDescEntry(
    struct NaClNameService  *self,
    char const              *name,
    int                     mode,
    struct NaClDesc         *new_desc);  /* takes ownership of ref */

typedef int (*NaClNameServiceFactoryFn_t)(void             *factory_state,
                                          char const       *name,
                                          int              flags,
                                          struct NaClDesc  **out);

int NaClNameServiceCreateFactoryEntry(
    struct NaClNameService      *self,
    char const                  *name,
    /*
     * If a |name| corresponding to a factory is overwritten,
     * references to the factory and its state is just dropped/lost
     * after invoking the factory with a NULL for the |out| argument;
     * the factory function should decrement reference counts as
     * appropriate and clean up when this occurs.
     */
    NaClNameServiceFactoryFn_t  factory_fn,
    void                        *factory_state);

int NaClNameServiceResolveName(
    struct NaClNameService  *self,
    char const              *name,
    int                     flags,
    struct NaClDesc         **out);

int NaClNameServiceDeleteName(struct NaClNameService  *nnsp,
                              char const              *name);

size_t NaClNameServiceEnumerate(struct NaClNameService  *nnsp,
                                char                    *dest,
                                size_t                  nbytes);

struct NaClNameServiceVtbl {
  struct NaClSimpleServiceVtbl  vbase;
  /*
   * The following functions return 0 for success, and non-zero to
   * indicate the reason for failure.  The set of possible failures is
   * in src/trusted/service_runtime/include/sys/nacl_name_service.h.
   *
   * They don't really have to be virtual, but this makes it easier to
   * subclass and modify later, should the need occur.
   */
  int                           (*CreateDescEntry)(
      struct NaClNameService  *self,
      char const              *name,
      int                     mode,
      struct NaClDesc         *new_desc);  /* takes ownership of ref */
  int                           (*CreateFactoryEntry)(
      struct NaClNameService      *self,
      char const                  *name,
      NaClNameServiceFactoryFn_t  factory_fn,
      void                        *factory_state);
  int                           (*ResolveName)(
      struct NaClNameService  *self,
      char const              *name,
      int                     flags,
      struct NaClDesc         **out);
  int                           (*DeleteName)(
      struct NaClNameService  *self,
      char const              *name);

  /*
   * Enumerate always succeeds and returns the number of bytes
   * written.  To see if the buffer was large enough, check that the
   * number of bytes written is less than the supplied buffer size.
   * NB: if the number of bytes is equal to the buffer size, even if
   * the last byte is a NUL we cannot know that there aren't more
   * entries.  (A partially written entry is only one possibility; it
   * could happen that the initial entries exactly fit the buffer, but
   * there are more.)
   */
  size_t                        (*Enumerate)(
      struct NaClNameService  *self,
      char                    *dest,
      size_t                  nbytes);
};

extern struct NaClNameServiceVtbl kNaClNameServiceVtbl;

void NaClNameServiceLaunch(struct NaClNameService *self);

EXTERN_C_END

#endif
/* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NAME_SERVICE_NAME_SERVICE_H_ */
