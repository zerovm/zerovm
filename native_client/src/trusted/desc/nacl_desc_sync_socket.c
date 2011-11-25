/*
 * Copyright (c) 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_sync_socket.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#if NACL_WINDOWS
# include "native_client/src/shared/platform/win/xlate_system_error.h"
#endif

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"

/*
 * This file contains the implementation of the NaClDescSyncSocket
 * subclass of NaClDesc.
 *
 * NaClDescSyncSocket is the subclass that wraps base::SyncSocket descriptors.
 */

static struct NaClDescVtbl const kNaClDescSyncSocketVtbl;  /* fwd */

int NaClDescSyncSocketCtor(struct NaClDescSyncSocket  *self,
                           NaClHandle                 h) {
  int retval;
  retval = NaClDescCtor(&self->base);
  if (!retval) {
    return 0;
  }

  self->h = h;
  self->base.base.vtbl =
      (struct NaClRefCountVtbl const *) &kNaClDescSyncSocketVtbl;

  return retval;
}

static void NaClDescSyncSocketDtor(struct NaClRefCount *vself) {
  struct NaClDescSyncSocket  *self = ((struct NaClDescSyncSocket *) vself);

  (void) NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

static int NaClDescSyncSocketFstat(struct NaClDesc          *vself,
                                   struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFDSOCK;
  return 0;
}

static ssize_t NaClDescSyncSocketRead(struct NaClDesc          *vself,
                                      void                     *buf,
                                      size_t                   len) {
  struct NaClDescSyncSocket *self = (struct NaClDescSyncSocket *) vself;

  return NaClDescReadFromHandle(self->h, buf, len);
}

static ssize_t NaClDescSyncSocketWrite(struct NaClDesc         *vself,
                                       void const              *buf,
                                       size_t                  len) {
  struct NaClDescSyncSocket *self = (struct NaClDescSyncSocket *) vself;

  return NaClDescWriteToHandle(self->h, buf, len);
}


static int NaClDescSyncSocketExternalizeSize(struct NaClDesc  *vself,
                                             size_t           *nbytes,
                                             size_t           *nhandles) {
  UNREFERENCED_PARAMETER(vself);
  NaClLog(4, "Entered NaClDescSyncSocketExternalizeSize\n");
  *nbytes = 0;
  *nhandles = 1;

  return 0;
}

static int NaClDescSyncSocketExternalize(struct NaClDesc          *vself,
                                         struct NaClDescXferState *xfer) {
  struct NaClDescSyncSocket *self = ((struct NaClDescSyncSocket *) vself);

  NaClLog(4, "Entered NaClDescSyncSocketExternalize\n");
  *xfer->next_handle++ = self->h;
  return 0;
}


static struct NaClDescVtbl const kNaClDescSyncSocketVtbl = {
  {
    NaClDescSyncSocketDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescSyncSocketRead,
  NaClDescSyncSocketWrite,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescSyncSocketFstat,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_SYNC_SOCKET,
  NaClDescSyncSocketExternalizeSize,
  NaClDescSyncSocketExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};


int NaClDescSyncSocketInternalize(struct NaClDesc          **out_desc,
                                  struct NaClDescXferState *xfer) {
  int                       rv;
  NaClHandle                h;
  struct NaClDescSyncSocket *ndssp;

  NaClLog(4, "Entered NaClDescSyncSocketInternalize\n");
  rv = -NACL_ABI_EIO;
  h = NACL_INVALID_HANDLE;
  ndssp = NULL;

  if (xfer->next_handle == xfer->handle_buffer_end) {
    NaClLog(LOG_ERROR,
            ("NaClSyncSocketInternalize: no descriptor"
             " left in xfer state\n"));
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  ndssp = malloc(sizeof *ndssp);
  if (NULL == ndssp) {
    NaClLog(LOG_ERROR,
            "NaClSyncSocketInternalize: no memory\n");
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (!NaClDescSyncSocketCtor(ndssp, *xfer->next_handle)) {
    NaClLog(LOG_ERROR,
            "NaClSyncSocketInternalize: descriptor ctor error\n");
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  *out_desc = (struct NaClDesc *) ndssp;
  rv = 0;

cleanup:
  if (rv < 0) {
    free(ndssp);
  }
  return rv;
}
