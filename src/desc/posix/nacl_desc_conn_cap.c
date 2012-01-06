/*
 * Copyright 2010  The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.
 * Connection capabilities.
 */

#include <string.h>
#include <sys/socket.h>

#include "src/platform/nacl_log.h"
#include "src/desc/nacl_desc_conn_cap.h"
#include "src/desc/nacl_desc_imc.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/stat.h"

static struct NaClDescVtbl const kNaClDescConnCapFdVtbl;  /* fwd */

int NaClDescConnCapFdCtor(struct NaClDescConnCapFd  *self,
                          NaClHandle                endpt) {
  struct NaClDesc *basep = (struct NaClDesc *) self;

  basep->base.vtbl = (struct NaClRefCountVtbl const *) NULL;
  if (!NaClDescCtor(basep)) {
    return 0;
  }
  self->connect_fd = endpt;
  basep->base.vtbl = (struct NaClRefCountVtbl const *) &kNaClDescConnCapFdVtbl;
  return 1;
}


static void NaClDescConnCapFdDtor(struct NaClRefCount *vself) {
  struct NaClDescConnCapFd *self = (struct NaClDescConnCapFd *) vself;

  (void) NaClClose(self->connect_fd);
  self->connect_fd = NACL_INVALID_HANDLE;
  vself->vtbl = (struct NaClRefCountVtbl const *) &kNaClDescVtbl;
  (*vself->vtbl->Dtor)(vself);
  return;
}

static int NaClDescConnCapFdFstat(struct NaClDesc       *vself,
                                  struct nacl_abi_stat  *statbuf) {
  UNREFERENCED_PARAMETER(vself);

  memset(statbuf, 0, sizeof *statbuf);
  statbuf->nacl_abi_st_mode = NACL_ABI_S_IFSOCKADDR | NACL_ABI_S_IRWXU;
  return 0;
}

static int NaClDescConnCapFdExternalizeSize(struct NaClDesc *vself,
                                            size_t          *nbytes,
                                            size_t          *nhandles) {
  UNREFERENCED_PARAMETER(vself);

  *nbytes = 0;
  *nhandles = 1;

  return 0;
}

static int NaClDescConnCapFdExternalize(struct NaClDesc          *vself,
                                        struct NaClDescXferState *xfer) {
  struct NaClDescConnCapFd    *self;

  self = (struct NaClDescConnCapFd *) vself;
  *xfer->next_handle++ = self->connect_fd;

  return 0;
}

static int NaClDescConnCapFdConnectAddr(struct NaClDesc *vself,
                                        struct NaClDesc **out_desc) {
  struct NaClDescConnCapFd  *self = (struct NaClDescConnCapFd *) vself;
  NaClHandle                sock_pair[2];
  struct NaClDescImcDesc    *connected_socket;
  char                      control_buf[CMSG_SPACE(sizeof(int))];
  struct iovec              iovec;
  struct msghdr             connect_msg;
  struct cmsghdr            *cmsg;
  int                       sent;
  int                       retval;

  sock_pair[0] = NACL_INVALID_HANDLE;
  sock_pair[1] = NACL_INVALID_HANDLE;
  connected_socket = (struct NaClDescImcDesc *) NULL;

  retval = -NACL_ABI_EINVAL;

  if (0 != NaClSocketPair(sock_pair)) {
    retval = -NACL_ABI_EMFILE;
    goto cleanup;
  }

  iovec.iov_base = "c";
  iovec.iov_len = 1;
  connect_msg.msg_iov = &iovec;
  connect_msg.msg_iovlen = 1;
  connect_msg.msg_name = NULL;
  connect_msg.msg_namelen = 0;
  connect_msg.msg_control = control_buf;
  connect_msg.msg_controllen = sizeof(control_buf);
  connect_msg.msg_flags = 0;

  cmsg = CMSG_FIRSTHDR(&connect_msg);
  cmsg->cmsg_len = CMSG_LEN(sizeof(int));
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type = SCM_RIGHTS;
  /*
   * We use memcpy() rather than assignment through a cast to avoid
   * strict-aliasing warnings
   */
  memcpy(CMSG_DATA(cmsg), &sock_pair[0], sizeof(int));
  /* Set msg_controllen to the actual size of the cmsg. */
  connect_msg.msg_controllen = cmsg->cmsg_len;

  sent = sendmsg(self->connect_fd, &connect_msg, 0);
  if (1 != sent) {
    retval = -NACL_ABI_EIO;
    goto cleanup;
  }

  (void) NaClClose(sock_pair[0]);
  sock_pair[0] = NACL_INVALID_HANDLE;

  connected_socket = malloc(sizeof(*connected_socket));
  if (NULL == connected_socket ||
      !NaClDescImcDescCtor(connected_socket, sock_pair[1])) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }
  sock_pair[1] = NACL_INVALID_HANDLE;

  *out_desc = (struct NaClDesc *) connected_socket;
  connected_socket = NULL;
  retval = 0;

cleanup:
  NaClSafeCloseNaClHandle(sock_pair[0]);
  NaClSafeCloseNaClHandle(sock_pair[1]);
  free(connected_socket);

  return retval;
}

static int NaClDescConnCapFdAcceptConn(struct NaClDesc  *vself,
                                       struct NaClDesc  **out_desc) {
  UNREFERENCED_PARAMETER(vself);
  UNREFERENCED_PARAMETER(out_desc);

  NaClLog(LOG_ERROR, "NaClDescConnCapFdAcceptConn: not IMC\n");
  return -NACL_ABI_EINVAL;
}

static struct NaClDescVtbl const kNaClDescConnCapFdVtbl = {
  {
    NaClDescConnCapFdDtor,
  },
  NaClDescMapNotImplemented,
  NaClDescUnmapUnsafeNotImplemented,
  NaClDescUnmapNotImplemented,
  NaClDescReadNotImplemented,
  NaClDescWriteNotImplemented,
  NaClDescSeekNotImplemented,
  NaClDescIoctlNotImplemented,
  NaClDescConnCapFdFstat,
  NaClDescGetdentsNotImplemented,
  NACL_DESC_CONN_CAP_FD,
  NaClDescConnCapFdExternalizeSize,
  NaClDescConnCapFdExternalize,
  NaClDescLockNotImplemented,
  NaClDescTryLockNotImplemented,
  NaClDescUnlockNotImplemented,
  NaClDescWaitNotImplemented,
  NaClDescTimedWaitAbsNotImplemented,
  NaClDescSignalNotImplemented,
  NaClDescBroadcastNotImplemented,
  NaClDescSendMsgNotImplemented,
  NaClDescRecvMsgNotImplemented,
  NaClDescConnCapFdConnectAddr,
  NaClDescConnCapFdAcceptConn,
  NaClDescPostNotImplemented,
  NaClDescSemWaitNotImplemented,
  NaClDescGetValueNotImplemented,
};

int NaClDescConnCapFdInternalize(struct NaClDesc          **out_desc,
                                 struct NaClDescXferState *xfer) {
  struct NaClDescConnCapFd *conn_cap;

  if (xfer->next_handle == xfer->handle_buffer_end) {
    return -NACL_ABI_EIO;
  }
  conn_cap = malloc(sizeof(*conn_cap));
  if (NULL == conn_cap) {
    return -NACL_ABI_ENOMEM;
  }
  if (!NaClDescCtor(&conn_cap->base)) {
    free(conn_cap);
    return -NACL_ABI_ENOMEM;
  }
  conn_cap->base.base.vtbl =
      (struct NaClRefCountVtbl const *) &kNaClDescConnCapFdVtbl;
  conn_cap->connect_fd = *xfer->next_handle;
  *xfer->next_handle++ = NACL_INVALID_HANDLE;
  *out_desc = &conn_cap->base;
  return 0;
}
