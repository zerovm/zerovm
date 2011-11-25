/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <string.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"

#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_sync.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_quota.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nrd_xfer_intern.h"
#include "native_client/src/trusted/desc/pepper/nacl_pepper.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"

static struct NaClDescVtbl const kNaClDescQuotaVtbl;

int NaClDescQuotaCtor(struct NaClDescQuota  *self,
                      struct NaClDesc       *desc,
                      uint8_t const         *file_id) {
  if (!NaClDescCtor(&self->base)) {
    NACL_VTBL(NaClDescQuota, self) = NULL;
    return 0;
  }
  if (!NaClMutexCtor(&self->mu)) {
    /* do not NaClRefCountUnref, since we cannot free: caller must do that */
    (*NACL_VTBL(NaClRefCount, self)->Dtor)((struct NaClRefCount *) self);
    return 0;
  }
  self->desc = desc;  /* take ownership */
  memcpy(self->file_id, file_id, NACL_DESC_QUOTA_FILE_ID_LEN);
  NACL_VTBL(NaClDesc, self) = &kNaClDescQuotaVtbl;
  return 1;
}

void NaClDescQuotaDtor(struct NaClRefCount *vself) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  NaClRefCountUnref((struct NaClRefCount *) self->desc);
  self->desc = NULL;
  NaClMutexDtor(&self->mu);

  NACL_VTBL(NaClDesc, self) = &kNaClDescVtbl;
  (*NACL_VTBL(NaClRefCount, self)->Dtor)(vself);
}

uintptr_t NaClDescQuotaMap(struct NaClDesc          *vself,
                           struct NaClDescEffector  *effp,
                           void                     *start_addr,
                           size_t                   len,
                           int                      prot,
                           int                      flags,
                           nacl_off64_t             offset) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->
          Map)(self->desc, effp, start_addr, len, prot, flags, offset);
}

int NaClDescQuotaUnmapUnsafe(struct NaClDesc          *vself,
                             struct NaClDescEffector  *effp,
                             void                     *start_addr,
                             size_t                   len) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->
          UnmapUnsafe)(self->desc, effp, start_addr, len);
}

int NaClDescQuotaUnmap(struct NaClDesc          *vself,
                       struct NaClDescEffector  *effp,
                       void                     *start_addr,
                       size_t                   len) {
  struct NaClDescQuota *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->
          Unmap)(self->desc, effp, start_addr, len);
}

ssize_t NaClDescQuotaRead(struct NaClDesc *vself,
                          void            *buf,
                          size_t          len) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Read)(self->desc, buf, len);
}

ssize_t NaClDescQuotaWrite(struct NaClDesc  *vself,
                           void const       *buf,
                           size_t           len) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  nacl_off64_t          file_offset;
  uint64_t              len_u64;
  int64_t               allowed;
  ssize_t               rv;

  NaClXMutexLock(&self->mu);
  if (0 == len) {
    allowed = 0;
  } else {
    /*
     * prevent another thread from doing a repositioning seek between the
     * lseek(d,0,1) and the Write.
     */
    file_offset = (*NACL_VTBL(NaClDesc, self->desc)->Seek)(self->desc,
                                                           0,
                                                           SEEK_CUR);
    if (file_offset < 0) {
      rv = (ssize_t) file_offset;
      goto abort;
    }

    NACL_COMPILE_TIME_ASSERT(SIZE_T_MAX <= NACL_UMAX_VAL(uint64_t));
    /*
     * Write can always return a short, non-zero transfer count.
     */
    len_u64 = (uint64_t) len;
    /* get rid of the always-true/always-false comparison warning */
    if (len_u64 > NACL_MAX_VAL(int64_t)) {
      len = (size_t) NACL_MAX_VAL(int64_t);
    }

    allowed = NaClSrpcPepperWriteRequest(self->file_id,
                                         file_offset,
                                         (int64_t) len);
    if (allowed <= 0) {
      rv = -NACL_ABI_EDQUOT;
      goto abort;
    }
    /*
     * allowed <= len should be a post-condition, but we check for
     * it anyway.
     */
    if ((uint64_t) allowed > len) {
      NaClLog(LOG_WARNING,
              ("NaClSrpcPepperWriteRequest returned a allowed quota that"
               " is larger than that requested; reducing to original"
               " request amount.\n"));
      allowed = len;
    }
  }

  rv = (*NACL_VTBL(NaClDesc, self->desc)->Write)(self->desc,
                                                 buf, (size_t) allowed);
abort:
  NaClXMutexUnlock(&self->mu);
  return rv;
}

nacl_off64_t NaClDescQuotaSeek(struct NaClDesc  *vself,
                               nacl_off64_t     offset,
                               int              whence) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  nacl_off64_t          rv;

  NaClXMutexLock(&self->mu);
  rv = (*NACL_VTBL(NaClDesc, self->desc)->Seek)(self->desc, offset, whence);
  NaClXMutexUnlock(&self->mu);

  return rv;
}

int NaClDescQuotaIoctl(struct NaClDesc  *vself,
                       int              request,
                       void             *arg) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Ioctl)(self->desc, request, arg);
}

int NaClDescQuotaFstat(struct NaClDesc      *vself,
                       struct nacl_abi_stat *statbuf) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Fstat)(self->desc, statbuf);
}

ssize_t NaClDescQuotaGetdents(struct NaClDesc *vself,
                              void            *dirp,
                              size_t          count) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  return (*NACL_VTBL(NaClDesc, self->desc)->Getdents)(self->desc, dirp, count);
}

int NaClDescQuotaExternalizeSize(struct NaClDesc *vself,
                                 size_t          *nbytes,
                                 size_t          *nhandles) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;
  int                   rv;
  size_t                num_bytes;
  size_t                num_handles;

  if (0 != (rv = (*NACL_VTBL(NaClDesc, vself)->
                  ExternalizeSize)(vself, &num_bytes, &num_handles))) {
    return rv;
  }
  *nbytes = num_bytes + sizeof self->file_id;
  *nhandles = num_handles;
  NaClNrdXferIncrTagOverhead(nbytes, nhandles);
  return 0;
}

/*
 * nrd_xfer tagging scheme details escapes into here.
 */
int NaClDescQuotaExternalize(struct NaClDesc          *vself,
                             struct NaClDescXferState *xfer) {
  struct NaClDescQuota  *self = (struct NaClDescQuota *) vself;

  memcpy(xfer->next_byte, self->file_id, sizeof self->file_id);
  xfer->next_byte += sizeof self->file_id;

  if (0 != NaClDescExternalizeToXferBuffer(xfer, vself)) {
    NaClLog(LOG_ERROR,
            ("NaClDescQuotaExternalize: externalizing wrapped descriptor"
             " type %d failed\n"),
            NACL_VTBL(NaClDesc, vself)->typeTag);
    return -NACL_ABI_EINVAL;  /* invalid/non-transferable desc type */
  }
  return 0;
}

int NaClDescQuotaInternalize(struct NaClDesc          **out_desc,
                             struct NaClDescXferState *xfer) {
  int                   rv = -NACL_ABI_EIO;
  uint8_t               file_id[NACL_DESC_QUOTA_FILE_ID_LEN];
  struct NaClDescQuota  *out = NULL;
  struct NaClDesc       *wrapped_desc;

  if (NULL == (out = malloc(sizeof *out))) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  memcpy(file_id, xfer->next_byte, sizeof file_id);
  xfer->next_byte += sizeof file_id;

  if (1 != NaClDescInternalizeFromXferBuffer(&wrapped_desc, xfer)) {
    rv = -NACL_ABI_EIO;
    goto cleanup;
  }
  if (!NaClDescQuotaCtor(out, wrapped_desc, file_id)) {
    rv = -NACL_ABI_ENOMEM;
    goto cleanup_wrapped;

  }

  *out_desc = (struct NaClDesc *) out;
  rv = 0;

cleanup_wrapped:
  if (0 != rv) {
    NaClDescUnref(wrapped_desc);
  }

cleanup:
  if (0 != rv) {
    free(out);
  }
  return rv;
}

int NaClDescQuotaLock(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->Lock)(vself);
}

int NaClDescQuotaTryLock(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->TryLock)(vself);
}

int NaClDescQuotaUnlock(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->Unlock)(vself);
}

int NaClDescQuotaWait(struct NaClDesc *vself,
                      struct NaClDesc *mutex) {
  return (*NACL_VTBL(NaClDesc, vself)->Wait)(vself, mutex);
}

int NaClDescQuotaTimedWaitAbs(struct NaClDesc                *vself,
                              struct NaClDesc                *mutex,
                              struct nacl_abi_timespec const *ts) {
  return (*NACL_VTBL(NaClDesc, vself)->TimedWaitAbs)(vself, mutex, ts);
}

int NaClDescQuotaSignal(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->Signal)(vself);
}

int NaClDescQuotaBroadcast(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->Broadcast)(vself);
}

ssize_t NaClDescQuotaSendMsg(struct NaClDesc                *vself,
                             struct NaClMessageHeader const *dgram,
                             int                            flags) {
  return (*NACL_VTBL(NaClDesc, vself)->SendMsg)(vself, dgram, flags);
}

ssize_t NaClDescQuotaRecvMsg(struct NaClDesc          *vself,
                             struct NaClMessageHeader *dgram,
                             int                      flags) {
  return (*NACL_VTBL(NaClDesc, vself)->RecvMsg)(vself, dgram, flags);
}

int NaClDescQuotaConnectAddr(struct NaClDesc *vself,
                             struct NaClDesc **result) {
  return (*NACL_VTBL(NaClDesc, vself)->ConnectAddr)(vself, result);
}

int NaClDescQuotaAcceptConn(struct NaClDesc *vself,
                            struct NaClDesc **result) {
  return (*NACL_VTBL(NaClDesc, vself)->AcceptConn)(vself, result);
}

int NaClDescQuotaPost(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->Post)(vself);
}

int NaClDescQuotaSemWait(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->SemWait)(vself);
}

int NaClDescQuotaGetValue(struct NaClDesc *vself) {
  return (*NACL_VTBL(NaClDesc, vself)->GetValue)(vself);
}


static struct NaClDescVtbl const kNaClDescQuotaVtbl = {
  {
    NaClDescQuotaDtor,
  },
  NaClDescQuotaMap,
  NaClDescQuotaUnmapUnsafe,
  NaClDescQuotaUnmap,
  NaClDescQuotaRead,
  NaClDescQuotaWrite,
  NaClDescQuotaSeek,
  NaClDescQuotaIoctl,
  NaClDescQuotaFstat,
  NaClDescQuotaGetdents,
  NACL_DESC_QUOTA,
  NaClDescQuotaExternalizeSize,
  NaClDescQuotaExternalize,
  NaClDescQuotaLock,
  NaClDescQuotaTryLock,
  NaClDescQuotaUnlock,
  NaClDescQuotaWait,
  NaClDescQuotaTimedWaitAbs,
  NaClDescQuotaSignal,
  NaClDescQuotaBroadcast,
  NaClDescQuotaSendMsg,
  NaClDescQuotaRecvMsg,
  NaClDescQuotaConnectAddr,
  NaClDescQuotaAcceptConn,
  NaClDescQuotaPost,
  NaClDescQuotaSemWait,
  NaClDescQuotaGetValue,
};
