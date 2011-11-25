/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "native_client/src/include/portability.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/shared/platform/nacl_check.h"
/*
 * #including nacl_srpc_message.h before nacl_host_desc.h currently
 * fails with nacl-glibc because it leaves nacl_abi_time_t undeclared.
 * TODO(mseaborn): Fix problems with these headers.
 */
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/srpc/nacl_srpc_message.h"
#include "native_client/src/shared/srpc/nacl_srpc.h"
#include "native_client/src/shared/srpc/nacl_srpc_internal.h"

#ifdef __native_client__
/*
 * We cannot currently #include service_runtime/include/sys/errno.h
 * with nacl-glibc because it #includes sys/reent.h, which is a
 * newlibism that glibc does not provide.
 * TODO(mseaborn): Fix problems with these headers.
 */
# include <errno.h>
/**
 * Note: nacl/nacl_inttypes.h must be included last...
 * after all other types headers.
 */
#include <machine/_types.h>
# define NACL_ABI_RECVMSG_DATA_TRUNCATED RECVMSG_DATA_TRUNCATED
# define NACL_ABI_RECVMSG_DESC_TRUNCATED RECVMSG_DESC_TRUNCATED
# define NACL_ABI_EIO EIO
# define NACL_ABI_EINVAL EINVAL
#include "native_client/src/trusted/service_runtime/include/sys/nacl_syscalls.h"
#else
# include "native_client/src/trusted/desc/nrd_xfer_effector.h"
# include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#endif

#ifndef SIZE_T_MAX
# define SIZE_T_MAX (~((size_t) 0))
#endif

static size_t size_min(size_t a, size_t b) {
  if (a < b) {
    return a;
  } else {
    return b;
  }
}

/*
 * Defines for the executable portions of the differences between trusted
 * and untrusted code.
 */
#ifdef __native_client__

static const NaClSrpcMessageDesc   kInvalidDesc = -1;

static ssize_t ImcSendmsg(NaClSrpcMessageDesc desc,
                          const NaClSrpcMessageHeader* header,
                          int flags) {
  nacl_abi_ssize_t retval = imc_sendmsg(desc, header, flags);
  if (-1 == retval) {
    return -errno;
  }
  return retval;
}

static ssize_t ImcRecvmsg(NaClSrpcMessageDesc desc,
                          NaClSrpcMessageHeader* header,
                          int flags) {
  nacl_abi_ssize_t retval = imc_recvmsg(desc, header, flags);
  if (-1 == retval) {
    return -errno;
  }
  return retval;
}

#else  /* trusted code */

/* These are defined by default in untrusted code. */
# define IMC_USER_DESC_MAX         NACL_ABI_IMC_USER_DESC_MAX
# define IMC_USER_BYTES_MAX        NACL_ABI_IMC_USER_BYTES_MAX
static const NaClSrpcMessageDesc   kInvalidDesc = NULL;

static ssize_t ImcSendmsg(NaClSrpcMessageDesc desc,
                          const NaClSrpcMessageHeader* header,
                          int flags) {
  return NaClImcSendTypedMessage(desc, header, flags);
}

static ssize_t ImcRecvmsg(NaClSrpcMessageDesc desc,
                          NaClSrpcMessageHeader* header,
                          int flags) {
  return NaClImcRecvTypedMessage(desc, header, flags);
}

#endif  /* __native_client__ */

static const size_t kDescSize = sizeof(NaClSrpcMessageDesc);

struct PortableDesc {
#ifndef __native_client__
  /*
   * An interface class used to enable passing of descs in trusted
   * code, such as the browser plugin.
   */
  struct NaClNrdXferEffector eff;
  struct NaClDescEffector* effp;
#endif
  NaClSrpcMessageDesc raw_desc;
};

/*
 * A wrapper class for NaClSrpcMessageDesc that allows clients to ignore
 * implementation differences.
 */
static int PortableDescCtor(struct PortableDesc* self,
                            NaClSrpcMessageDesc desc) {
#ifndef __native_client__
  self->effp = NULL;
#endif  /* __native_client__ */
  if (kInvalidDesc == desc) {
    return 0;
  }
#ifdef __native_client__
  self->raw_desc = desc;
#else
  self->raw_desc = NaClDescRef(desc);
  if (!NaClNrdXferEffectorCtor(&self->eff)) {
    self->raw_desc = kInvalidDesc;
    return 0;
  }
  self->effp = (struct NaClDescEffector*) &self->eff;
#endif  /* __native_client__ */
  return 1;
}

static void PortableDescDtor(struct PortableDesc* self) {
#ifndef __native_client__
  if (NULL != self->effp) {
    self->effp->vtbl->Dtor(self->effp);
  }
  self->effp = NULL;
  NaClDescSafeUnref(self->raw_desc);
#endif  /* __native_client__ */
  self->raw_desc = kInvalidDesc;
}

/*
 * The length descriptor type for messages.  Messages may be fragmented into
 * multiple calls to ImcSendmsg/ImcRecvmsg.  The first fragment contains two
 * LengthHeaders; one gives the total byte and desc count, and the other
 * indicates the first fragment's byte and desc count.  All subsequent
 * fragments contain just the fragment's byte and desc count.  These need to
 * be platform-independent types, because the sender and receiver may have
 * a different notion of how big size_t is.
 */
typedef struct {
  nacl_abi_size_t byte_count;
  nacl_abi_size_t desc_count;
} LengthHeader;
#define FRAGMENT_OVERHEAD ((ssize_t) (sizeof(LengthHeader)))

struct NaClSrpcMessageChannel {
  struct PortableDesc desc;
  /* The below members are used to buffer a single message, for use by peek. */
  char bytes[IMC_USER_BYTES_MAX];
  size_t byte_count;
  NaClSrpcMessageDesc descs[IMC_USER_DESC_MAX];
  size_t desc_count;
};

struct NaClSrpcMessageChannel* NaClSrpcMessageChannelNew(
    NaClSrpcMessageDesc desc) {
  struct NaClSrpcMessageChannel* channel = NULL;

  channel = (struct NaClSrpcMessageChannel*) malloc(sizeof *channel);
  if (NULL == channel) {
    return NULL;
  }
  if (!PortableDescCtor(&channel->desc, desc)) {
    free(channel);
    return NULL;
  }
  channel->byte_count = 0;
  channel->desc_count = 0;
  return channel;
}

void NaClSrpcMessageChannelDelete(struct NaClSrpcMessageChannel* channel) {
  if (NULL != channel) {
    PortableDescDtor(&channel->desc);
    free(channel);
  }
}

/*
 * Read the next fragment of a message into channel's buffer.
 */
static uint32_t MessageChannelBufferFirstFragment(
    struct NaClSrpcMessageChannel* channel) {
  ssize_t imc_ret = -1;
  NaClSrpcMessageHeader buffer_header;
  struct NaClImcMsgIoVec iovec[1];

  NaClSrpcLog(3,
              "MessageChannelBufferFirstFragment: waiting for message.\n");
  /* Read the entire first fragment into channel's buffer. */
  buffer_header.iov = iovec;
  buffer_header.iov_length = NACL_ARRAY_SIZE(iovec);
  buffer_header.NACL_SRPC_MESSAGE_HEADER_DESCV = channel->descs;
  buffer_header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH =
      NACL_ARRAY_SIZE(channel->descs);
  buffer_header.iov[0].base = channel->bytes;
  buffer_header.iov[0].length = NACL_ARRAY_SIZE(channel->bytes);
  buffer_header.flags = 0;
  /*
   * The message receive should return at least 2* FRAGMENT_OVERHEAD bytes,
   * one for the message header and one for the first fragment header.
   */
  imc_ret = ImcRecvmsg(channel->desc.raw_desc, &buffer_header, 0);
  if ((2 * FRAGMENT_OVERHEAD > imc_ret) || (buffer_header.flags != 0)) {
    NaClSrpcLog(3,
                "MessageChannelBufferFirstFragment: read failed (%"
                NACL_PRIdS").\n",
                imc_ret);
    return 0;
  }
  NaClSrpcLog(3,
              "MessageChannelBufferFirstFragment: buffered message: "
              "bytes %"NACL_PRIdS", descs %"NACL_PRIuS".\n",
              imc_ret,
              (size_t) buffer_header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  channel->byte_count = imc_ret;
  channel->desc_count = buffer_header.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
  return 1;
}

/*
 * Read from channel's buffer.
 */
static ssize_t MessageChannelBufferRead(struct NaClSrpcMessageChannel* channel,
                                        NaClSrpcMessageHeader* header,
                                        int peeking) {
  size_t i;
  size_t byte_count = 0;
  size_t iov_read_size;
  size_t descv_read_count;

  /*
   * If there are no bytes or descriptors in the buffer, fill the buffer
   * by reading the first fragment.
   */
  if (channel->byte_count == 0 && channel->desc_count == 0) {
    if (!peeking) {
      /* A read with an empty buffer just reads. */
      return ImcRecvmsg(channel->desc.raw_desc, header, 0);
    }
    /* Peeking needs to read the first fragment into the buffer. */
    if (!MessageChannelBufferFirstFragment(channel)) {
      NaClSrpcLog(3,
                  "MessageChannelBufferRead: couldn't buffer.\n");
      return -1;
    }
  }
  header->flags = 0;
  NaClSrpcLog(3,
              "MessageChannelBufferRead: channel->byte_count=%"NACL_PRIdS".\n",
              channel->byte_count);
  for (i = 0; i < header->iov_length; ++i) {
    NaClSrpcLog(3,
                "MessageChannelBufferRead: bytes %"NACL_PRIdS" chan %"
                NACL_PRIdS".\n",
                byte_count,
                channel->byte_count);
    if (channel->byte_count < byte_count) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "MessageChannelBufferRead: overflow.\n");
      return -1;
    }
    iov_read_size =
        size_min(channel->byte_count - byte_count, header->iov[i].length);
    if (SIZE_T_MAX - byte_count < iov_read_size) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "MessageChannelBufferRead: overflow.\n");
      return -1;
    }
    memcpy(header->iov[i].base, channel->bytes + byte_count, iov_read_size);
    byte_count += iov_read_size;
    if (byte_count == channel->byte_count) {
      /* We have read the entire contents of the buffer. */
      NaClSrpcLog(3,
                  "MessageChannelBufferRead: break\n");
      break;
    }
  }
  if (byte_count < channel->byte_count) {
    header->flags |= NACL_ABI_RECVMSG_DATA_TRUNCATED;
  }
  descv_read_count =
      size_min(channel->desc_count,
               header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  /* channel->desc_count <= NACL_ABI_SIZE_T_MAX, so casts are safe. */
  if (SIZE_T_MAX / sizeof(NaClSrpcMessageDesc) < descv_read_count) {
    /* Descriptor descv_read_count * sizeof would overflow. */
    return -1;
  }
  memcpy(header->NACL_SRPC_MESSAGE_HEADER_DESCV,
         channel->descs,
         descv_read_count * sizeof(NaClSrpcMessageDesc));
  header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = (nacl_abi_size_t)
      descv_read_count;
  if (descv_read_count < channel->desc_count) {
    header->flags |= NACL_ABI_RECVMSG_DESC_TRUNCATED;
  }
  /* Reading clears the fragment from the buffer. */
  if (!peeking) {
    channel->byte_count = 0;
    channel->desc_count = 0;
  }
  return (ssize_t) byte_count;
}

static ssize_t IovTotalBytes(const struct NaClImcMsgIoVec* iov,
                             size_t iov_length,
                             size_t entries_to_skip) {
  size_t i;
  size_t byte_count = 0;
  for (i = entries_to_skip; i < iov_length; ++i) {
    if (SIZE_T_MAX - iov[i].length < byte_count) {
      return -1;
    }
    byte_count += iov[i].length;
  }
  /* Clamp the result to be representable as a nacl_abi_ssize_t. */
  if (NACL_ABI_SSIZE_T_MAX < byte_count) {
    return -1;
  }
  return (ssize_t) byte_count;
}

static ssize_t HeaderTotalBytes(const NaClSrpcMessageHeader* header,
                                size_t entries_to_skip) {
  return IovTotalBytes(header->iov, header->iov_length, entries_to_skip);
}

static int ComputeFragmentSizes(const NaClSrpcMessageHeader* header,
                                size_t entries_to_skip,
                                LengthHeader* fragment_size) {
  size_t byte_count = (size_t) HeaderTotalBytes(header, entries_to_skip);
  if (-1 == (ssize_t) byte_count) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "ComputeFragmentSizes: byte_count was incorrect.\n");
    return 0;
  }
  if (0 == kNaClSrpcMaxImcSendmsgSize) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "ComputeFragmentSizes: NaClSrpcModuleInit not called.\n");
    return 0;
  }
  /* kNaClSrpcMaxImcSendmsgSize <= NACL_ABI_SIZE_T_MAX, so cast is safe. */
  fragment_size->byte_count = (nacl_abi_size_t)
      size_min(byte_count, kNaClSrpcMaxImcSendmsgSize);
  /* SRPC_DESC_MAX <= NACL_ABI_SIZE_T_MAX, so cast is safe. */
  fragment_size->desc_count = (nacl_abi_size_t)
      size_min(SRPC_DESC_MAX, header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  return 1;
}

/*
 * Send and receive destructively update the iov.  This builds a copy that
 * can be used to destructively update, including adding the entries that
 * are prepended for message lengths.
 */
static struct NaClImcMsgIoVec* CopyAndAddIovs(const struct NaClImcMsgIoVec* iov,
                                              size_t iov_length,
                                              size_t extra_entries) {
  struct NaClImcMsgIoVec* copy;

  /* Check for arithmetic overflows. */
  if (SIZE_T_MAX - iov_length < extra_entries ||
      SIZE_T_MAX / sizeof *iov < iov_length + extra_entries) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "CopyAndAddIovs: overflows.\n");
    return NULL;
  }
  /* Check for total bytes exceeding NACL_ABI_SSIZE_T_MAX. */
  if (-1 == IovTotalBytes(iov, iov_length, extra_entries)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "CopyAndAddIovs: total bytes overflows.\n");
    return NULL;
  }
  /* Copy the iov, adding extra_entries elements. */
  copy = (struct NaClImcMsgIoVec*) malloc((iov_length + extra_entries) *
                                          sizeof *iov);
  if (NULL == copy) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "CopyAndAddIovs: copy malloc failed.\n");
    return NULL;
  }
  memcpy(copy + extra_entries, iov, iov_length * sizeof *iov);
  return copy;
}

static int BuildFragmentHeader(NaClSrpcMessageHeader* header,
                               LengthHeader* fragment_size,
                               size_t entries_to_skip,
                               NaClSrpcMessageHeader* frag_hdr) {
  size_t i;
  size_t total_bytes = 0;
  const size_t kMaxIovEntries = SIZE_T_MAX / sizeof *frag_hdr->iov;

  if (NACL_ABI_SIZE_T_MAX < header->iov_length) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildFragmentHeader: iov_length too large.\n");
    return 0;
  }
  /* Copy the entire iovec, even though only part may be used. */
  frag_hdr->iov_length = header->iov_length;
  if (kMaxIovEntries < header->iov_length) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildFragmentHeader: iov_length > kMaxIovEntries.\n");
    return 0;
  }
  frag_hdr->iov = (struct NaClImcMsgIoVec*)
      malloc(header->iov_length * sizeof *frag_hdr->iov);
  if (frag_hdr->iov == NULL) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "BuildFragmentHeader: iov malloc failed.\n");
    return 0;
  }
  memcpy(frag_hdr->iov,
         header->iov,
         frag_hdr->iov_length * sizeof *frag_hdr->iov);
  /* Update the iov[i].length entries. */
  for (i = entries_to_skip; i < header->iov_length; ++i) {
    size_t bytes_this_iov =
        size_min(fragment_size->byte_count - total_bytes,
                 frag_hdr->iov[i].length);
    if (bytes_this_iov == 0) {
      /* header->iov_length was checked at entry to make this safe. */
      frag_hdr->iov_length = (nacl_abi_size_t) i;
    }
    frag_hdr->iov[i].length = bytes_this_iov;
    /* Ensure that total_bytes increment doesn't overflow. */
    if (SIZE_T_MAX - bytes_this_iov < total_bytes) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "BuildFragmentHeader: total bytes overflows.\n");
      return 0;
    }
    total_bytes += bytes_this_iov;
  }
  frag_hdr->NACL_SRPC_MESSAGE_HEADER_DESCV =
      header->NACL_SRPC_MESSAGE_HEADER_DESCV;
  frag_hdr->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = fragment_size->desc_count;
  return 1;
}

static void ConsumeFragment(NaClSrpcMessageHeader* header,
                            LengthHeader* fragment_size,
                            size_t guaranteed_entries) {
  size_t descs_read;
  /*
   * The caller has already checked that the number of bytes read is sufficient
   * to ensure that the first "guaranteed_entries" iov entries were satisfied.
   * guaranteed_entries is passed as a constant 1 or 2, so cast is safe.
   */
  header->iov += (nacl_abi_size_t) guaranteed_entries;
  header->iov_length -= (nacl_abi_size_t) guaranteed_entries;
  /* Update to reflect the fragment's descriptors that were consumed. */
  descs_read =
      size_min(fragment_size->desc_count,
               header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  /* Post-condition: descs_read <= NACL_ABI_SIZE_T_MAX. */
  header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH -= (nacl_abi_size_t) descs_read;
  header->NACL_SRPC_MESSAGE_HEADER_DESCV += (nacl_abi_size_t) descs_read;
  fragment_size->desc_count -= (nacl_abi_size_t) descs_read;
  /*
   * Update the header and iov vector to reflect which entries are already
   * satisfied.
   */
  while ((header->iov_length > 0) && (fragment_size->byte_count > 0)) {
    size_t bytes_for_this_entry;
    bytes_for_this_entry =
        size_min(header->iov[0].length, fragment_size->byte_count);
    /* Post-condition: bytes_for_this_entry <= NACL_ABI_SIZE_T_MAX. */
    header->iov[0].length -= (nacl_abi_size_t) bytes_for_this_entry;
    fragment_size->byte_count -= (nacl_abi_size_t) bytes_for_this_entry;
    if (header->iov[0].length > 0) {
      /* The fragment was exhausted, but didn't satisfy this iov entry. */
      header->iov[0].base = (char*) header->iov[0].base + bytes_for_this_entry;
      break;
    }
    /* This iov entry was satisfied. Remove it from the vector. */
    header->iov++;
    header->iov_length--;
  }
  if (fragment_size->byte_count > 0) {
    header->flags |= NACL_ABI_RECVMSG_DATA_TRUNCATED;
  }
  if (fragment_size->desc_count > 0) {
    header->flags |= NACL_ABI_RECVMSG_DESC_TRUNCATED;
  }
}

static int32_t FragmentLengthIsSane(LengthHeader* fragment_size,
                                    size_t bytes_received,
                                    size_t descs_received) {
  /*
   * bytes_received and descs_received are dependent on the message headers
   * passed in from the client.  The client can request fewer than are present
   * in the fragment.
   */
  if (fragment_size->byte_count < bytes_received - FRAGMENT_OVERHEAD ||
      fragment_size->desc_count < descs_received) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "FragmentLengthIsSane: Descriptor mismatch:"
                " bytes %"NACL_PRIuNACL_SIZE" < %"NACL_PRIuS
                " or descs %"NACL_PRIuNACL_SIZE" < %"NACL_PRIuS".\n",
                fragment_size->byte_count,
                (bytes_received - FRAGMENT_OVERHEAD),
                fragment_size->desc_count,
                descs_received);
    return 0;
  }
  /*
   * Every fragment needs to pass at least one byte or one desc.
   * This ensures that each fragment is "making progress" towards finishing
   * the total message.
   */
  if (fragment_size->byte_count == FRAGMENT_OVERHEAD &&
      fragment_size->desc_count == 0) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "FragmentLengthIsSane: empty fragment. Terminating.\n");
    return 0;
  }
  return 1;
}

static int32_t MessageLengthsAreSane(LengthHeader* total_size,
                                     LengthHeader* fragment_size,
                                     size_t bytes_received,
                                     size_t descs_received) {
  /*
   * Empty messages are not allowed.
   */
  if (total_size->byte_count == 0 && total_size->desc_count == 0) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "MessageLengthsAreSane: Descriptor mismatch:"
                " bytes %"NACL_PRIdNACL_SIZE" == 0 or descs %"
                NACL_PRIdNACL_SIZE" == 0.\n",
                fragment_size->byte_count,
                fragment_size->desc_count);
    return 0;
  }
  /*
   * The first fragment must constitute a subset (not necessarily proper)
   * of the total message.
   */
  if (fragment_size->byte_count > total_size->byte_count ||
      fragment_size->desc_count > total_size->desc_count) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "MessageLengthsAreSane: Descriptor mismatch:"
                " bytes %"NACL_PRIdNACL_SIZE" > %"NACL_PRIdNACL_SIZE
                " or descs %"NACL_PRIdNACL_SIZE" > %"NACL_PRIdNACL_SIZE".\n",
                fragment_size->byte_count,
                total_size->byte_count,
                fragment_size->desc_count,
                total_size->desc_count);
    return 0;
  }
  /*
   * And the first fragment must be correct.
   */
  return FragmentLengthIsSane(fragment_size,
                              bytes_received - FRAGMENT_OVERHEAD,
                              descs_received);
}

static ssize_t ErrnoFromImcRet(ssize_t imc_ret) {
  if (0 > imc_ret) {
    return imc_ret;
  } else {
    return -NACL_ABI_EIO;
  }
}

/*
 * Peek a message from channel.  Reads the first fragment of the message and
 * leaves it available for future calls to Peek or Receive.
 */
ssize_t NaClSrpcMessageChannelPeek(struct NaClSrpcMessageChannel* channel,
                                   NaClSrpcMessageHeader* header) {
  /*
   * TODO(sehr): Most of this function is common with Receive.
   * Find a way to merge them.
   */
  struct NaClImcMsgIoVec* iovec = NULL;
  NaClSrpcMessageHeader header_copy;
  LengthHeader total_size;
  LengthHeader fragment_size;
  ssize_t imc_ret;
  ssize_t retval = -NACL_ABI_EINVAL;

  /* Append the fragment headers to the iov. */
  iovec = CopyAndAddIovs(header->iov, header->iov_length, 2);
  if (NULL == iovec) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelPeek: CopyAndAddIovs failed.\n");
    return -1;
  }
  header_copy.iov = iovec;
  header_copy.iov_length = header->iov_length + 2;
  header_copy.NACL_SRPC_MESSAGE_HEADER_DESCV =
      header->NACL_SRPC_MESSAGE_HEADER_DESCV;
  /* SRPC_DESC_MAX <= NACL_ABI_SIZE_T_MAX, so the cast is safe. */
  header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = (nacl_abi_size_t)
      size_min(SRPC_DESC_MAX, header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  header_copy.iov[0].base = &total_size;
  header_copy.iov[0].length = sizeof total_size;
  header_copy.iov[1].base = &fragment_size;
  header_copy.iov[1].length = sizeof fragment_size;
  header_copy.flags = 0;
  if (-1 == HeaderTotalBytes(&header_copy, 0)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelPeek: header size overflow.\n");
    goto done;
  }
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelPeek: read message bytes %"
              NACL_PRIdS", descs %"NACL_PRIdS".\n",
              channel->byte_count,
              channel->desc_count);
  imc_ret = MessageChannelBufferRead(channel, &header_copy, 1);
  if (2 * FRAGMENT_OVERHEAD > imc_ret) {
    NaClSrpcLog(3,
                "NaClSrpcMessageChannelPeek: read failed (%"NACL_PRIdS").\n",
                imc_ret);
    retval = ErrnoFromImcRet(imc_ret);
    goto done;
  }
  header->flags = header_copy.flags;
  header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH =
      header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelPeek: flags %x.\n",
              header->flags);
  if (!MessageLengthsAreSane(
           &total_size,
           &fragment_size,
           (size_t) imc_ret,
           header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelPeek: message length mismatch.\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  retval = imc_ret - 2 * FRAGMENT_OVERHEAD;

 done:
  free(iovec);
  return retval;
}

/*
 * Receive a message from channel.  On success it returns the number of
 * bytes read; otherwise, returns -1.
 */
ssize_t NaClSrpcMessageChannelReceive(struct NaClSrpcMessageChannel* channel,
                                      NaClSrpcMessageHeader* header) {
  /*
   * TODO(sehr): A large prefix of this function is common with Peek.
   * Find a way to merge them.
   */
  ssize_t imc_ret = -1;
  NaClSrpcMessageHeader header_copy;
  struct NaClImcMsgIoVec* iovec = NULL;
  LengthHeader total_size;
  LengthHeader fragment_size;
  LengthHeader processed_size;
  size_t bytes_received;
  size_t descs_received;
  ssize_t retval = -NACL_ABI_EINVAL;

  NaClSrpcLog(3,
              "NaClSrpcMessageChannelReceive: waiting for message.\n");
  /*
   * The first fragment consists of two LengthHeaders and a fraction of the
   * bytes (starting at 0) and the fraction of descs (starting at 0).
   */
  iovec = CopyAndAddIovs(header->iov, header->iov_length, 2);
  if (NULL == iovec) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelReceive: CopyAndAddIovs failed.\n");
    goto done;
  }
  header_copy.iov = iovec;
  header_copy.iov_length = header->iov_length + 2;
  header_copy.NACL_SRPC_MESSAGE_HEADER_DESCV =
      header->NACL_SRPC_MESSAGE_HEADER_DESCV;
  /* SRPC_DESC_MAX <= NACL_ABI_SIZE_T_MAX, so the cast is safe. */
  header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = (nacl_abi_size_t)
      size_min(SRPC_DESC_MAX, header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH);
  header_copy.iov[0].base = &total_size;
  header_copy.iov[0].length = sizeof total_size;
  header_copy.iov[1].base = &fragment_size;
  header_copy.iov[1].length = sizeof fragment_size;
  header_copy.flags = 0;
  if (-1 == HeaderTotalBytes(&header_copy, 0)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelReceive: header size overflow.\n");
    goto done;
  }
  /*
   * The message receive should return at least 2* FRAGMENT_OVERHEAD bytes,
   * one for the message header and one for the first fragment header.
   */
  imc_ret = MessageChannelBufferRead(channel, &header_copy, 0);
  if (2 * FRAGMENT_OVERHEAD > imc_ret) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelReceive: read failed (%"NACL_PRIdS").\n",
                imc_ret);
    retval = ErrnoFromImcRet(imc_ret);
    goto done;
  }
  bytes_received = imc_ret - 2 * FRAGMENT_OVERHEAD;
  descs_received = header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
  if (!MessageLengthsAreSane(
          &total_size,
          &fragment_size,
          (size_t) imc_ret,
          header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelReceive:"
                " first fragment descriptor check failed.\n");
    retval = -NACL_ABI_EIO;
    goto done;
  }
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelReceive:"
              " new message, bytes %"NACL_PRIdNACL_SIZE
              ", descs %"NACL_PRIdNACL_SIZE".\n",
              total_size.byte_count,
              total_size.desc_count);
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelReceive:"
              " first fragment, bytes %"NACL_PRIdNACL_SIZE
              ", descs %"NACL_PRIdNACL_SIZE".\n",
              fragment_size.byte_count,
              fragment_size.desc_count);
  processed_size = fragment_size;
  ConsumeFragment(&header_copy, &fragment_size, 2);
  /*
   * Get the remaining fragments.
   */
  while (processed_size.byte_count < total_size.byte_count ||
         processed_size.desc_count < total_size.desc_count) {
    /*
     * The non-first fragments consist of a single LengthHeader and a
     * portion of the remaining iov entries and descv entries.  We add the
     * fragment length descriptor to the preceding iov entry, which is safe,
     * because we know that ConsumeFragment always consumes at least the
     * fragment length descriptor from last time.
     */
    header_copy.iov = header_copy.iov - 1;
    header_copy.iov_length = header_copy.iov_length + 1;
    header_copy.iov[0].base = &fragment_size;
    header_copy.iov[0].length = sizeof fragment_size;
    header_copy.NACL_SRPC_MESSAGE_HEADER_DESCV =
        header->NACL_SRPC_MESSAGE_HEADER_DESCV + descs_received;
    header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH = (nacl_abi_size_t)
        size_min(SRPC_DESC_MAX,
                 (header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH -
                  descs_received));
    if (-1 == HeaderTotalBytes(&header_copy, 0)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelReceive: header size overflow.\n");
      goto done;
    }
    /*
     * The message receive should return at least FRAGMENT_OVERHEAD bytes.
     * This is needed to make sure that we can correctly maintain the index
     * into bytes and descs.
     */
    imc_ret = ImcRecvmsg(channel->desc.raw_desc, &header_copy, 0);
    if (imc_ret < FRAGMENT_OVERHEAD) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelReceive: read failed (%"
                  NACL_PRIdS").\n",
                  imc_ret);
      retval = ErrnoFromImcRet(imc_ret);
      goto done;
    }
    bytes_received += imc_ret - FRAGMENT_OVERHEAD;
    descs_received += header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
    if (!FragmentLengthIsSane(
            &fragment_size,
            (size_t) imc_ret,
            header_copy.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelReceive:"
                  " other fragment descriptor check failed.\n");
      retval = -NACL_ABI_EIO;
      goto done;
    }
    NaClSrpcLog(3,
                "NaClSrpcMessageChannelReceive:"
                " next fragment, bytes %"NACL_PRIdNACL_SIZE
                ", descs %"NACL_PRIdNACL_SIZE".\n",
                fragment_size.byte_count,
                fragment_size.desc_count);
    processed_size.byte_count += fragment_size.byte_count;
    processed_size.desc_count += fragment_size.desc_count;
    ConsumeFragment(&header_copy, &fragment_size, 1);
  }
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelReceive:"
              " succeeded, read %"NACL_PRIdS" bytes and %"
              NACL_PRIdNACL_SIZE" descs.\n",
              bytes_received,
              processed_size.desc_count);
  retval = (ssize_t) bytes_received;
  header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH =
      (nacl_abi_size_t) descs_received;
  header->flags = header_copy.flags;

 done:
  free(iovec);
  return retval;
}

/*
 * Sends message over the channel. It returns 1 if successful, or 0 otherwise.
 */
ssize_t NaClSrpcMessageChannelSend(struct NaClSrpcMessageChannel* channel,
                                   const NaClSrpcMessageHeader* header) {
  ssize_t imc_ret;
  struct NaClImcMsgIoVec* iovec = NULL;
  NaClSrpcMessageHeader remaining;
  NaClSrpcMessageHeader frag_hdr;
  LengthHeader total_size;
  LengthHeader fragment_size;
  size_t expected_bytes_sent;
  ssize_t retval = -NACL_ABI_EINVAL;

  iovec = CopyAndAddIovs(header->iov, header->iov_length, 2);
  if (NULL == iovec) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend: CopyAndAddIovs failed.\n");
    goto done;
  }
  remaining.iov = iovec;
  remaining.iov_length = header->iov_length + 2;
  remaining.NACL_SRPC_MESSAGE_HEADER_DESCV =
      header->NACL_SRPC_MESSAGE_HEADER_DESCV;
  remaining.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH =
      header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
  remaining.iov[0].base = &total_size;
  remaining.iov[0].length = sizeof total_size;
  remaining.iov[1].base = &fragment_size;
  remaining.iov[1].length = sizeof fragment_size;
  if (-1 == HeaderTotalBytes(&remaining, 0)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend: header size overflow.\n");
    goto done;
  }
  /*
   * Send the first (possibly only) fragment.
   * HeaderTotalBytes returns -1 if the total is greater than
   * NACL_ABI_SSIZE_T_MAX.
   */
  total_size.byte_count = (nacl_abi_size_t) HeaderTotalBytes(&remaining, 2);
  if (-1 == (nacl_abi_ssize_t) total_size.byte_count) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend: HeaderTotalBytes failed.\n");
    goto done;
  }
  total_size.desc_count = header->NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH;
  /*
   * Compute the first fragment's message descriptor and fragment descriptor,
   * limiting the bytes and descriptors sent in the first fragment to preset
   * amounts.
   */
  if (!ComputeFragmentSizes(&remaining, 2, &fragment_size)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend:"
                " first ComputeFragmentSize failed.\n");
    goto done;
  }
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelSend: new message, bytes %"
              NACL_PRIdNACL_SIZE", descs %"NACL_PRIdNACL_SIZE".\n",
              total_size.byte_count,
              total_size.desc_count);
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelSend: first fragment, bytes %"
              NACL_PRIdNACL_SIZE", descs %"NACL_PRIdNACL_SIZE".\n",
              fragment_size.byte_count,
              fragment_size.desc_count);
  expected_bytes_sent = fragment_size.byte_count + 2 * FRAGMENT_OVERHEAD;
  if (!BuildFragmentHeader(&remaining, &fragment_size, 2, &frag_hdr)) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend:"
                " could not build fragment header.\n");
    goto done;
  }
  /*
   * The first message has at least three iov entries: one for the (message)
   * total_size descriptor, one for the fragment_size descriptor, and at
   * least one for the first fragment's bytes and descs.
   */
  imc_ret = ImcSendmsg(channel->desc.raw_desc, &frag_hdr, 0);
  free(frag_hdr.iov);
  if ((size_t) imc_ret != expected_bytes_sent) {
    NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                "NaClSrpcMessageChannelSend: first send failed, %"
                NACL_PRIdS" != %"NACL_PRIdS".\n",
                expected_bytes_sent,
                imc_ret);
    retval = ErrnoFromImcRet(imc_ret);
    goto done;
  }
  ConsumeFragment(&remaining, &fragment_size, 2);
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelSend: first send succeeded.\n");
  /*
   * Each subsequent fragment contains the bytes starting at next_byte and
   * the descs starting at next_desc.
   */
  while (remaining.iov_length > 0 ||
         remaining.NACL_SRPC_MESSAGE_HEADER_DESC_LENGTH > 0) {
    NaClSrpcMessageHeader frag_hdr;
    /*
     * Each subsequent message has two iov entries: one for the fragment_size
     * descriptor, and one for the fragment's bytes and descs.
     * We add the fragment length descriptor to the preceding iov entry,
     * which is safe, because we know that ConsumeFragment always consumes
     * at least the fragment length descriptor from last time.
     */
    remaining.iov = remaining.iov - 1;
    remaining.iov_length = remaining.iov_length + 1;
    remaining.iov[0].base = &fragment_size;
    remaining.iov[0].length = sizeof fragment_size;
    if (-1 == HeaderTotalBytes(&remaining, 0)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelSend: header size overflow.\n");
      goto done;
    }
    /*
     * The fragment sizes are again limited.
     */
    if (!ComputeFragmentSizes(&remaining, 1, &fragment_size)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelSend:"
                  " other ComputeFragmentSize failed.\n");
      retval = -NACL_ABI_EIO;
      goto done;
    }
    NaClSrpcLog(3,
                "NaClSrpcMessageChannelSend: next fragment, bytes %"
                NACL_PRIdNACL_SIZE", descs %"NACL_PRIdNACL_SIZE".\n",
                fragment_size.byte_count,
                fragment_size.desc_count);
    if (!BuildFragmentHeader(&remaining, &fragment_size, 1, &frag_hdr)) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelSend:"
                  " could not build fragment header.\n");
      retval = -NACL_ABI_EIO;
      goto done;
    }
    /*
     * Send the fragment.
     */
    expected_bytes_sent = fragment_size.byte_count + FRAGMENT_OVERHEAD;
    imc_ret = ImcSendmsg(channel->desc.raw_desc, &frag_hdr, 0);
    free(frag_hdr.iov);
    if ((size_t) imc_ret != expected_bytes_sent) {
      NaClSrpcLog(NACL_SRPC_LOG_ERROR,
                  "NaClSrpcMessageChannelSend: send error.\n");
      retval = ErrnoFromImcRet(imc_ret);
      goto done;
    }
    ConsumeFragment(&remaining, &fragment_size, 1);
  }
  NaClSrpcLog(3,
              "NaClSrpcMessageChannelSend: complete send, sent %"
              NACL_PRIdNACL_SIZE" bytes and %"NACL_PRIdNACL_SIZE" descs.\n",
              total_size.byte_count,
              total_size.desc_count);
  retval = (ssize_t) total_size.byte_count;

 done:
  free(iovec);
  return retval;
}
