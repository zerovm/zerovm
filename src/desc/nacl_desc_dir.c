/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  Directory descriptor abstraction.
 */

#include <string.h>
#include "src/desc/nacl_desc_dir.h"
#include "src/platform/nacl_host_dir.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/include/sys/dirent.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/stat.h"

// ###
///*
// * This file contains the implementation for the NaClDirDesc subclass
// * of NaClDesc.
// *
// * NaClDescDirDesc is the subclass that wraps host-OS directory information.
// */
//
//static struct NaClDescVtbl const kNaClDescDirDescVtbl;  /* fwd */

// ###
//static void NaClDescDirDescDtor(struct NaClRefCount *vself) {
//  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;
//
//  NaClLog(4, "NaClDescDirDescDtor(0x%08"NACL_PRIxPTR").\n",
//          (uintptr_t) vself);
//  NaClHostDirClose(self->hd);
//  free(self->hd);
//  self->hd = NULL;
//  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
//  (*vself->vtbl->Dtor)(vself);
//}

// ###
//static ssize_t NaClDescDirDescGetdents(struct NaClDesc         *vself,
//                                       void                    *dirp,
//                                       size_t                  count) {
//  struct NaClDescDirDesc *self = (struct NaClDescDirDesc *) vself;
//  struct nacl_abi_dirent *direntp = (struct nacl_abi_dirent *) dirp;
//  ssize_t retval;
//
//  NaClLog(3, "NaClDescDirDescGetdents(0x%08"NACL_PRIxPTR", %"NACL_PRIuS"):\n",
//          (uintptr_t) dirp, count);
//  retval = NaClHostDirGetdents(self->hd, dirp, count);
//  NaClLog(3,
//          "NaClDescDirDescGetdents(d_ino=%"NACL_PRIuNACL_INO", "
//          "d_off=%"NACL_PRIuNACL_OFF", d_reclen=%u, "
//          "d_name='%s')\n",
//          direntp->nacl_abi_d_ino,
//          direntp->nacl_abi_d_off,
//          direntp->nacl_abi_d_reclen,
//          direntp->nacl_abi_d_name);
//  return retval;
//}

// ###
//static ssize_t NaClDescDirDescRead(struct NaClDesc         *vself,
//                                   void                    *buf,
//                                   size_t                  len) {
//  /* NaClLog(LOG_ERROR, "NaClDescDirDescRead: Read not allowed on dir\n"); */
//  return NaClDescDirDescGetdents(vself, buf, len);
//  /* return -NACL_ABI_EINVAL; */
//}

// ###
//static int NaClDescDirDescFstat(struct NaClDesc          *vself,
//                                struct nacl_abi_stat     *statbuf) {
//  UNREFERENCED_PARAMETER(vself);
//
//  memset(statbuf, 0, sizeof *statbuf);
//  /*
//   * TODO(bsy): saying it's executable/searchable might be a lie.
//   */
//  statbuf->nacl_abi_st_mode = (NACL_ABI_S_IFDIR |
//                               NACL_ABI_S_IRUSR |
//                               NACL_ABI_S_IXUSR);
//  return 0;
//}

// ###
//static int NaClDescDirDescExternalizeSize(struct NaClDesc *vself,
//                                          size_t          *nbytes,
//                                          size_t          *nhandles) {
//  UNREFERENCED_PARAMETER(vself);
//  *nbytes = 0;
//  *nhandles = 1;
//  return 0;
//}

// ###
//static struct NaClDescVtbl const kNaClDescDirDescVtbl = {
//  {
//    NaClDescDirDescDtor,
//  },
//  NaClDescMapNotImplemented,
//  NaClDescUnmapUnsafeNotImplemented,
//  NaClDescUnmapNotImplemented,
//  NaClDescDirDescRead,
//  NaClDescWriteNotImplemented,
//  NaClDescSeekNotImplemented,
//  NaClDescIoctlNotImplemented,
//  NaClDescDirDescFstat,
//  NaClDescDirDescGetdents,
//  NACL_DESC_DIR,
//  NaClDescDirDescExternalizeSize,
//  NaClDescExternalizeNotImplemented,
//  NaClDescLockNotImplemented,
//  NaClDescTryLockNotImplemented,
//  NaClDescUnlockNotImplemented,
//  NaClDescWaitNotImplemented,
//  NaClDescTimedWaitAbsNotImplemented,
//  NaClDescSignalNotImplemented,
//  NaClDescBroadcastNotImplemented,
//  NaClDescSendMsgNotImplemented,
//  NaClDescRecvMsgNotImplemented,
//  NaClDescConnectAddrNotImplemented,
//  NaClDescAcceptConnNotImplemented,
//  NaClDescPostNotImplemented,
//  NaClDescSemWaitNotImplemented,
//  NaClDescGetValueNotImplemented,
//};

// ###
//int NaClDescDirInternalize(struct NaClDesc           **out_desc,
//                           struct NaClDescXferState  *xfer) {
//  UNREFERENCED_PARAMETER(out_desc);
//  UNREFERENCED_PARAMETER(xfer);
//
//  NaClLog(LOG_ERROR, "NaClDescDirDescInternalize: not implemented for dir\n");
//  return -NACL_ABI_EINVAL;
//}
