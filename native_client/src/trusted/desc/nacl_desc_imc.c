/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "native_client/src/include/portability.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
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
 * This file contains the implementation of the NaClDescImcDesc
 * subclass of NaClDesc.
 *
 * NaClDescImcDesc is the subclass that wraps IMC socket descriptors.
 */

/* fwd */
static struct NaClDescVtbl const kNaClDescImcConnectedDescVtbl;
static struct NaClDescVtbl const kNaClDescImcDescVtbl;
static struct NaClDescVtbl const kNaClDescXferableDataDescVtbl;

int NaClDescImcConnectedDescCtor(struct NaClDescImcConnectedDesc  *self,
                                 NaClHandle                       h) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;

  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->h = h;
  basep->base.vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescImcConnectedDescVtbl;

  return 1;
}

static void NaClDescImcConnectedDescDtor(struct NaClRefCount *vself) {
  struct NaClDescImcConnectedDesc *self = ((struct NaClDescImcConnectedDesc *)
                                           vself);
  (void) NaClClose(self->h);
  self->h = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescImcDescCtor(struct NaClDescImcDesc  *self,
                        NaClHandle              h) {
  int retval;

  retval = NaClDescImcConnectedDescCtor(&self->base, h);
  if (!retval) {
    return 0;
  }
  if (!NaClMutexCtor(&self->sendmsg_mu)) {
    NaClDescUnref((struct NaClDesc *) self);
    return 0;
  }
  if (!NaClMutexCtor(&self->recvmsg_mu)) {
    NaClMutexDtor(&self->sendmsg_mu);
    NaClDescUnref((struct NaClDesc *) self);
    return 0;
  }
  self->base.base.base.vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescImcDescVtbl;

  return retval;
}

static void NaClDescImcDescDtor(struct NaClRefCount *vself) {
  struct NaClDescImcDesc *self = ((struct NaClDescImcDesc *)
                                  vself);
  NaClMutexDtor(&self->sendmsg_mu);
  NaClMutexDtor(&self->recvmsg_mu);

  vself->vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescImcConnectedDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescXferableDataDescCtor(struct NaClDescXferableDataDesc  *self,
                                 NaClHandle                       h) {
  int retval;
  retval = NaClDescImcConnectedDescCtor(&self->base, h);
  if (!retval) {
    return 0;
  }
  self->base.base.base.vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescXferableDataDescVtbl;

  return retval;
}

static void NaClDescXferableDataDescDtor(struct NaClRefCount *vself) {
  vself->vtbl = (struct NaClRefCountVtbl const *)
      &kNaClDescImcConnectedDescVtbl;
  (*vself->vtbl->Dtor)(vself);
}

int NaClDescImcDescFstat(struct NaClDesc          *vself,
                         struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = (NACL_ABI_S_IFSOCK |
                               NACL_ABI_S_IRUSR |
                               NACL_ABI_S_IWUSR);
  return 0;
}

static int NaClDescXferableDataDescFstat(struct NaClDesc          *vself,
                                         struct nacl_abi_stat     *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFDSOCK;
  return 0;
}

static int NaClDescXferableDataDescExternalizeSize(struct NaClDesc  *vself,
                                                   size_t           *nbytes,
                                                   size_t           *nhandles) {
  UNREFERENCED_PARAMETER(vself);
  NaClLog(4, "Entered NaClDescXferableDataDescExternalizeSize\n");
  *nbytes = 0;
  *nhandles = 1;

  return 0;
}

static int NaClDescXferableDataDescExternalize(struct NaClDesc          *vself,
                                               struct NaClDescXferState *xfer) {
  struct NaClDescXferableDataDesc *self = ((struct NaClDescXferableDataDesc *)
                                           vself);

  NaClLog(4, "Entered NaClDescXferableDataDescExternalize\n");
  *xfer->next_handle++ = self->base.h;
  return 0;
}


/*
 * In the level of NaClDescImcDescSendMsg, we do not know what
 * protocol is implemented by NaClSendDatagram (and indeed, in the
 * Windows implementation, the access rights transfer involves a more
 * complex protocol to get the peer process id).  Because the
 * underlying low-level IMC implementation does not provide thread
 * safety, this is the source of race conditions: two simultaneous
 * imc_sendmsg syscalls to the same descriptor could get their various
 * protcol bits interleaved, so the receiver will get garbled data
 * that cause the wrong underlying host OS descriptor to be made
 * available to the NaCl module.
 *
 * In order to address this issue, we (1) make descriptors that can
 * transfer other descriptors non-transferable, and (2) we use locking
 * so that only one thread can be performing (the low level bits of)
 * imc_msgsend at a time.  The non-transferability of such descriptors
 * addresses the multi-module scenario, as opposed to the
 * multi-threaded scenario, where a sender race cause receiver
 * confusion.
 */
static ssize_t NaClDescImcDescSendMsg(struct NaClDesc                *vself,
                                      struct NaClMessageHeader const *dgram,
                               int                            flags) {
  struct NaClDescImcDesc *self = ((struct NaClDescImcDesc *)
                                  vself);
  int result;

  NaClXMutexLock(&self->sendmsg_mu);
  result = NaClSendDatagram(self->base.h, dgram, flags);
  NaClXMutexUnlock(&self->sendmsg_mu);

  if (-1 == result) {
#if NACL_WINDOWS
    return -NaClXlateSystemError(GetLastError());
#elif NACL_LINUX || NACL_OSX
    return -NaClXlateErrno(errno);
#else
# error "Unknown target platform: cannot translate error code(s) from SendMsg"
#endif
  }
  return result;
}


/*
 * NaClDescXferableDataDescSendMsg implements imc_sendmsg For
 * data-only descriptors.  We assume that whatever protocol exists at
 * the NaClSendDatagram level is still not thread safe, but that the
 * lack of thread safety will not have a significant impact on
 * security.  This is still somewhat brittle: the low-level Windows
 * IMC code can be convinced to do the wrong thing still via sender
 * races, but the receiver, because it expects zero handles, will have
 * called ReceiveDatagram in such a way that any "received handles"
 * are closed.  This implies that arbitrary Windows handles can be
 * made to be closed, including those not accessible by NaCl modules,
 * but fortunately this should only result in a denial of service as
 * the error caused by the use of an invalid handle is detected by the
 * service runtime and cause an abort.
 *
 * Note that it is still an application error to send or receive data
 * with a transferable data-only descriptor from two threads (or two
 * modules) simultaneously.
 */
static ssize_t
NaClDescXferableDataDescSendMsg(struct NaClDesc                *vself,
                                struct NaClMessageHeader const *dgram,
                                int                            flags) {
  struct NaClDescXferableDataDesc *self = ((struct NaClDescXferableDataDesc *)
                                           vself);
  int result;

  if (0 != dgram->handle_count) {
    /*
     * A transferable descriptor cannot be used to transfer other
     * descriptors.
     */
    NaClLog(2,
            ("NaClDescXferableDataDescSendMsg: tranferable and"
             " non-zero handle_count\n"));
    return -NACL_ABI_EINVAL;
  }

  result = NaClSendDatagram(self->base.h, dgram, flags);

  if (-1 == result) {
#if NACL_WINDOWS
    return -NaClXlateSystemError(GetLastError());
#elif NACL_LINUX || NACL_OSX
    return -NaClXlateErrno(errno);
#else
# error "Unknown target platform: cannot translate error code(s) from SendMsg"
#endif
  }
  return result;
}


/*
 * See discussion at NaClDescImcDescSendMsg for details.  An
 * imc_recvmsg race is not substantively different from an imc_sendmsg
 * race.
 */
static ssize_t NaClDescImcDescRecvMsg(struct NaClDesc          *vself,
                                      struct NaClMessageHeader *dgram,
                                      int                      flags) {
  struct NaClDescImcDesc *self = ((struct NaClDescImcDesc *)
                                  vself);
  int result;

  NaClLog(4, "Entered NaClDescImcDescRecvMsg, h=%d\n", self->base.h);
  NaClXMutexLock(&self->recvmsg_mu);
  result = NaClReceiveDatagram(self->base.h, dgram, flags);
  NaClXMutexUnlock(&self->recvmsg_mu);

  if (-1 == result) {
#if NACL_WINDOWS
    return -NaClXlateSystemError(GetLastError());
#elif NACL_LINUX || NACL_OSX
    return -errno;
#else
# error "Unknown target platform: cannot translate error code(s) from RecvMsg"
#endif
  }
  return result;
}


/*
 * See discussion at NaClDescXferableDataDescSendMsg for details.  An
 * imc_recvmsg race is not substantively different from an imc_sendmsg
 * race.
 */
static ssize_t NaClDescXferableDataDescRecvMsg(struct NaClDesc          *vself,
                                               struct NaClMessageHeader *dgram,
                                               int                      flags) {
  struct NaClDescXferableDataDesc *self = ((struct NaClDescXferableDataDesc *)
                                           vself);
  int                             result;

  NaClLog(4, "Entered NaClDescXferableDataDescRecvMsg, h = %d\n", self->base.h);
  if (0 != dgram->handle_count) {
    /*
     * A transferable descriptor is data-only, and it is an error to
     * try to receive any I/O descriptors with it.
     */
    NaClLog(2,
            "NaClDescXferableDataDescRecvMsg:"
            " tranferable and non-zero handle_count\n");
    return -NACL_ABI_EINVAL;
  }

  result = NaClReceiveDatagram(self->base.h, dgram, flags);

  if (-1 == result) {
#if NACL_WINDOWS
    return -NaClXlateSystemError(GetLastError());
#elif NACL_LINUX || NACL_OSX
    return -errno;
#else
# error "Unknown target platform: cannot translate error code(s) from RecvMsg"
#endif
  }
  return result;
}


static struct NaClDescVtbl const kNaClDescImcConnectedDescVtbl = {
  {
    NaClDescImcConnectedDescDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescFstatNotImplemented,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_CONNECTED_SOCKET,
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
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


static struct NaClDescVtbl const kNaClDescImcDescVtbl = {
  {
    NaClDescImcDescDtor,  /* diff */
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescImcDescFstat,  /* diff */
  NaClDescGetdentsNotImplemented,
  NACL_DESC_IMC_SOCKET,  /* diff */
  NaClDescExternalizeSizeNotImplemented,
  NaClDescExternalizeNotImplemented,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescImcDescSendMsg,  /* diff */
  NaClDescImcDescRecvMsg,  /* diff */
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};


static struct NaClDescVtbl const kNaClDescXferableDataDescVtbl = {
  {
    NaClDescXferableDataDescDtor,  /* diff */
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescXferableDataDescFstat,  /* diff */
  NaClDescGetdentsNotImplemented,
  NACL_DESC_TRANSFERABLE_DATA_SOCKET,  /* diff */
  NaClDescXferableDataDescExternalizeSize,  /* diff */
  NaClDescXferableDataDescExternalize,  /* diff */
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescXferableDataDescSendMsg,  /* diff */
  NaClDescXferableDataDescRecvMsg,  /* diff */
  NaClDescConnectAddrNotImplemented,
  NaClDescAcceptConnNotImplemented,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};


int NaClDescXferableDataDescInternalize(struct NaClDesc          **baseptr,
                                        struct NaClDescXferState *xfer) {
  int                             rv;
  NaClHandle                      h;
  struct NaClDescXferableDataDesc *ndxdp;

  NaClLog(4, "Entered NaClDescXferableDataDescInternalize\n");
  rv = -NACL_ABI_EIO;
  h = NACL_INVALID_HANDLE;
  ndxdp = NULL;

  if (xfer->next_handle == xfer->handle_buffer_end) {
    NaClLog(LOG_ERROR,
            ("NaClXferableDataDescInternalize: no descriptor"
             " left in xfer state\n"));
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  ndxdp = malloc(sizeof *ndxdp);
  if (NULL == ndxdp) {
    NaClLog(LOG_ERROR,
            "NaClXferableDataDescInternalize: no memory\n");
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  if (!NaClDescXferableDataDescCtor(ndxdp,
                                    *xfer->next_handle)) {
    NaClLog(LOG_ERROR,
            "NaClXferableDataDescInternalize: descriptor ctor error\n");
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  *baseptr = (struct NaClDesc *) ndxdp;
  rv = 0;

cleanup:
  if (rv < 0) {
    free(ndxdp);
  }
  return rv;
}
