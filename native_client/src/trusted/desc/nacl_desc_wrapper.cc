// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <limits>
#include <new>
#include "base/shared_memory.h"
#include "base/sync_socket.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/include/portability_string.h"
#include "native_client/src/shared/imc/nacl_imc.h"
#include "native_client/src/shared/platform/nacl_check.h"
#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"
#if defined(NACL_LINUX)
#include "native_client/src/trusted/desc/linux/nacl_desc_sysv_shm.h"
#endif  // defined(NACL_LINUX)
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_conn_cap.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/desc/nacl_desc_invalid.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_sync_socket.h"
#include "native_client/src/trusted/desc/nacl_desc_wrapper.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"
#include "native_client/src/trusted/desc/nrd_xfer_effector.h"
#include "native_client/src/trusted/nacl_base/nacl_refcount.h"
#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/nacl_imc_api.h"
#include "native_client/src/trusted/service_runtime/nacl_config.h"

// TODO(polina): follow the style guide and replace "nhdp" and "ndiodp" with
//               "host_desc" and "io_desc"

namespace {
static const size_t kSizeTMax = std::numeric_limits<size_t>::max();
}  // namespace

namespace nacl {

// Descriptor creation and manipulation sometimes requires additional state
// (for instance, Effectors).  Therefore, we create an object that encapsulates
// that state.
class DescWrapperCommon {
  friend class DescWrapperFactory;

 public:
  typedef uint32_t RefCountType;

  // Get a pointer to the effector.
  struct NaClDescEffector* effp() {
    return reinterpret_cast<struct NaClDescEffector*>(&eff_);
  }

  // Inform clients that the object was successfully initialized.
  bool is_initialized() const { return is_initialized_; }

  // Manipulate reference count
  DescWrapperCommon* Ref() {
    // TODO(sehr): replace with a reference count class when we have one.
    NaClXMutexLock(&ref_count_mu_);
    if (std::numeric_limits<RefCountType>::max() == ref_count_) {
      NaClLog(LOG_FATAL, "DescWrapperCommon ref count overflow\n");
    }
    ++ref_count_;
    NaClXMutexUnlock(&ref_count_mu_);
    return this;
  }

  void Unref() {
    NaClXMutexLock(&ref_count_mu_);
    if (0 == ref_count_) {
      NaClLog(LOG_FATAL, "DescWrapperCommon ref count already zero\n");
    }
    --ref_count_;
    bool destroy = (0 == ref_count_);
    NaClXMutexUnlock(&ref_count_mu_);
    if (destroy) {
      delete this;
    }
  }

 private:
  DescWrapperCommon() : is_initialized_(false), ref_count_(1) {
    NaClXMutexCtor(&ref_count_mu_);
  }
  ~DescWrapperCommon() {
    if (is_initialized_) {
      effp()->vtbl->Dtor(effp());
    }
    NaClMutexDtor(&ref_count_mu_);
  }

  // Set up the state.  Returns true on success.
  bool Init();

  // Boolean to indicate the object was successfully initialized.
  bool is_initialized_;
  // Effector for transferring descriptors.
  struct NaClNrdXferEffector eff_;
  // The reference count and the mutex to protect it.
  RefCountType ref_count_;
  struct NaClMutex ref_count_mu_;

  DISALLOW_COPY_AND_ASSIGN(DescWrapperCommon);
};

bool DescWrapperCommon::Init() {
  // Set up the transfer effector.
  if (!NaClNrdXferEffectorCtor(&eff_)) {
    return false;
  }
  // Successfully initialized.
  is_initialized_ = true;
  return true;
}

DescWrapperFactory::DescWrapperFactory() {
  common_data_ = new(std::nothrow) DescWrapperCommon();
  if (NULL == common_data_) {
    return;
  }
  if (!common_data_->Init()) {
    delete common_data_;
    common_data_ = NULL;
  }
}

DescWrapperFactory::~DescWrapperFactory() {
  if (NULL != common_data_) {
    common_data_->Unref();
  }
}

int DescWrapperFactory::MakeBoundSock(DescWrapper* pair[2]) {
  CHECK(common_data_->is_initialized());

  struct NaClDesc* descs[2] = { NULL, NULL };
  DescWrapper* tmp_pair[2] = { NULL, NULL };

  int ret = NaClCommonDescMakeBoundSock(descs);
  if (0 != ret) {
    return ret;
  }
  tmp_pair[0] = new(std::nothrow) DescWrapper(common_data_, descs[0]);
  if (NULL == tmp_pair[0]) {
    goto cleanup;
  }
  descs[0] = NULL;  // DescWrapper took ownership of descs[0].
  tmp_pair[1] = new(std::nothrow) DescWrapper(common_data_, descs[1]);
  if (NULL == tmp_pair[1]) {
    goto cleanup;
  }
  descs[1] = NULL;  // DescWrapper took ownership of descs[1].
  pair[0] = tmp_pair[0];
  pair[1] = tmp_pair[1];
  return 0;

 cleanup:
  NaClDescSafeUnref(descs[0]);
  NaClDescSafeUnref(descs[1]);
  delete tmp_pair[0];
  delete tmp_pair[1];
  return -1;
}

DescWrapper* DescWrapperFactory::MakeImcSock(NaClHandle handle) {
  struct NaClDescImcDesc* desc =
    reinterpret_cast<NaClDescImcDesc*>(calloc(1, sizeof *desc));
  if (NULL == desc) {
    return NULL;
  }
  if (!NaClDescImcDescCtor(desc, handle)) {
    free(desc);
    return NULL;
  }

  return MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(desc));
}

DescWrapper* DescWrapperFactory::MakeShm(size_t size) {
  CHECK(common_data_->is_initialized());
  // HACK: there's an inlining issue with this.
  // size_t rounded_size = NaClRoundAllocPage(size);
  size_t rounded_size =
      (size + NACL_MAP_PAGESIZE - 1) &
      ~static_cast<size_t>(NACL_MAP_PAGESIZE - 1);
  // TODO(sehr): fix the inlining issue.
  NaClHandle handle = CreateMemoryObject(rounded_size, /* executable= */ false);
  if (kInvalidHandle == handle) {
    return NULL;
  }
  return ImportShmHandle(handle, size);
}

DescWrapper* DescWrapperFactory::ImportShmHandle(NaClHandle handle,
                                                 size_t size) {
  struct NaClDescImcShm* desc =
    reinterpret_cast<NaClDescImcShm*>(calloc(1, sizeof *desc));
  if (NULL == desc) {
    return NULL;
  }
  if (!NaClDescImcShmCtor(desc, handle, size)) {
    free(desc);
    return NULL;
  }

  return MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(desc));
}

DescWrapper* DescWrapperFactory::ImportSyncSocketHandle(NaClHandle handle) {
  struct NaClDescSyncSocket* desc =
    static_cast<NaClDescSyncSocket*>(calloc(1, sizeof *desc));
  if (NULL == desc) {
    return NULL;
  }
  if (!NaClDescSyncSocketCtor(desc, handle)) {
    free(desc);
    return NULL;
  }

  return MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(desc));
}

#if NACL_LINUX
DescWrapper* DescWrapperFactory::ImportSysvShm(int key, size_t size) {
  if (kSizeTMax - NACL_PAGESIZE + 1 <= size) {
    // Avoid overflow when rounding to the nearest 4K and casting to
    // nacl_off64_t by preventing negative size.
    return NULL;
  }
  // HACK: there's an inlining issue with using NaClRoundPage. (See above.)
  // rounded_size = NaClRoundPage(size);
  size_t rounded_size =
      (size + NACL_PAGESIZE - 1) & ~static_cast<size_t>(NACL_PAGESIZE - 1);
  struct NaClDescSysvShm* desc =
    reinterpret_cast<NaClDescSysvShm*>(calloc(1, sizeof *desc));
  if (NULL == desc) {
    return NULL;
  }

  if (!NaClDescSysvShmImportCtor(desc,
                                 key,
                                 static_cast<nacl_off64_t>(rounded_size))) {
    // If rounded_size is negative due to overflow from rounding, it will be
    // rejected here by NaClDescSysvShmImportCtor.
    free(desc);
    return NULL;
  }

  return
    MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(desc));
}
#endif  // NACL_LINUX

#if defined(NACL_STANDALONE)
DescWrapper* DescWrapperFactory::ImportPepperSharedMemory(intptr_t shm_int,
                                                          size_t size) {
  // PepperSharedMemory is only present in the Chrome hookup.
  UNREFERENCED_PARAMETER(shm_int);
  UNREFERENCED_PARAMETER(size);
  return NULL;
}

DescWrapper* DescWrapperFactory::ImportPepper2DSharedMemory(intptr_t shm_int) {
  // Pepper2DSharedMemory is only present in the Chrome hookup.
  UNREFERENCED_PARAMETER(shm_int);
  return NULL;
}

DescWrapper* DescWrapperFactory::ImportPepperSync(intptr_t sync_int) {
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_WIN)
  base::SyncSocket* sock = reinterpret_cast<base::SyncSocket*>(sync_int);
  struct NaClDescSyncSocket* ss_desc = NULL;
  DescWrapper* wrapper = NULL;

  ss_desc = static_cast<NaClDescSyncSocket*>(
      calloc(1, sizeof(*ss_desc)));
  if (NULL == ss_desc) {
    // TODO(sehr): Gotos are awful.  Add a scoped_ptr variant that
    // invokes NaClDescSafeUnref.
    goto cleanup;
  }
  if (!NaClDescSyncSocketCtor(ss_desc, sock->handle())) {
    free(ss_desc);
    ss_desc = NULL;
    goto cleanup;
  }
  wrapper = new DescWrapper(common_data_, &ss_desc->base);
  if (NULL == wrapper) {
    goto cleanup;
  }
  ss_desc = NULL;  // DescWrapper takes ownership of ss_desc.
  return wrapper;

 cleanup:
  NaClDescSafeUnref(&ss_desc->base);
  return NULL;
#else
  UNREFERENCED_PARAMETER(sync_int);
  return NULL;
#endif
}
#endif  // NACL_STANDALONE

DescWrapper* DescWrapperFactory::MakeGeneric(struct NaClDesc* desc) {
  CHECK(common_data_->is_initialized());
  return new(std::nothrow) DescWrapper(common_data_, desc);
}


DescWrapper* DescWrapperFactory::MakeGenericCleanup(struct NaClDesc* desc) {
  CHECK(common_data_->is_initialized());
  DescWrapper* wrapper = new(std::nothrow) DescWrapper(common_data_, desc);
  if (NULL != wrapper) {
      return wrapper;
  }
  NaClDescSafeUnref(desc);
  return NULL;
}

int DescWrapperFactory::MakeSocketPair(DescWrapper* pair[2]) {
  CHECK(common_data_->is_initialized());
  struct NaClDesc* descs[2] = { NULL, NULL };
  DescWrapper* tmp_pair[2] = { NULL, NULL };

  int ret = NaClCommonDescSocketPair(descs);
  if (0 != ret) {
    return ret;
  }
  tmp_pair[0] = new(std::nothrow) DescWrapper(common_data_, descs[0]);
  if (NULL == tmp_pair[0]) {
    goto cleanup;
  }
  descs[0] = NULL;  // DescWrapper took ownership of descs[0].
  tmp_pair[1] = new(std::nothrow) DescWrapper(common_data_, descs[1]);
  if (NULL == tmp_pair[1]) {
    goto cleanup;
  }
  descs[1] = NULL;  // DescWrapper took ownership of descs[1].
  pair[0] = tmp_pair[0];
  pair[1] = tmp_pair[1];
  return 0;

 cleanup:
  NaClDescSafeUnref(descs[0]);
  NaClDescSafeUnref(descs[1]);
  delete tmp_pair[0];
  delete tmp_pair[1];
  return -1;
}

DescWrapper* DescWrapperFactory::MakeFileDesc(int host_os_desc, int mode) {
  NaClHostDesc* host_desc = NaClHostDescPosixMake(host_os_desc, mode);
  CHECK(host_desc != NULL);  // Otherwise Make() would have aborted.
  NaClDescIoDesc* io_desc = NaClDescIoDescMake(host_desc);
  CHECK(io_desc != NULL);  // Otherwise Make() would have aborted.
  NaClDesc* desc = reinterpret_cast<struct NaClDesc*>(io_desc);

  // The wrapper takes ownership of NaClDesc and hence NaClDescIoDesc,
  // which already took ownership of NaClHostDesc.
  return MakeGenericCleanup(desc);
}

DescWrapper* DescWrapperFactory::OpenHostFile(const char* fname,
                                              int flags,
                                              int mode) {
  struct NaClHostDesc* nhdp =
    reinterpret_cast<struct NaClHostDesc*>(calloc(1, sizeof(*nhdp)));
  if (NULL == nhdp) {
    return NULL;
  }
  if (0 != NaClHostDescOpen(nhdp, fname, flags, mode)) {
    free(nhdp);
    return NULL;
  }
  struct NaClDescIoDesc* ndiodp = NaClDescIoDescMake(nhdp);
  if (NULL == ndiodp) {
    NaClHostDescClose(nhdp);
    free(nhdp);
    return NULL;
  }

  return MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(ndiodp));
}

DescWrapper* DescWrapperFactory::MakeInvalid() {
  struct NaClDescInvalid *desc =
      const_cast<NaClDescInvalid*>(NaClDescInvalidMake());
  if (NULL == desc) {
    return NULL;
  }

  return MakeGenericCleanup(reinterpret_cast<struct NaClDesc*>(desc));
}

DescWrapper::DescWrapper(DescWrapperCommon* common_data,
                         struct NaClDesc* desc)
      : common_data_(common_data), desc_(desc) {
  // DescWrapper takes ownership of desc from caller, so no Ref call here.
  if (NULL != common_data_) {
    common_data_->Ref();
  }
}

DescWrapper::~DescWrapper() {
  if (NULL != common_data_) {
    common_data_->Unref();
  }
  NaClDescSafeUnref(desc_);
  desc_ = NULL;
}

int DescWrapper::Map(void** addr, size_t* size) {
  return NaClDescMapDescriptor(desc_, common_data_->effp(), addr, size);
}

int DescWrapper::Unmap(void* start_addr, size_t len) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Unmap(desc_,
            common_data_->effp(),
            start_addr, len);
}

ssize_t DescWrapper::Read(void* buf, size_t len) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Read(desc_, buf, len);
}

ssize_t DescWrapper::Write(const void* buf, size_t len) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Write(desc_, buf, len);
}

nacl_off64_t DescWrapper::Seek(nacl_off64_t offset, int whence) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Seek(desc_,
           offset,
           whence);
}

int DescWrapper::Ioctl(int request, void* arg) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Ioctl(desc_, request, arg);
}

int DescWrapper::Fstat(struct nacl_abi_stat* statbuf) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Fstat(desc_, statbuf);
}

int DescWrapper::Close() {
  NaClRefCountUnref(&desc_->base);
  return 0;
}

ssize_t DescWrapper::Getdents(void* dirp, size_t count) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Getdents(desc_,
               dirp,
               count);
}

int DescWrapper::Lock() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Lock(desc_);
}

int DescWrapper::TryLock() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      TryLock(desc_);
}

int DescWrapper::Unlock() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Unlock(desc_);
}

int DescWrapper::Wait(DescWrapper* mutex) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Wait(desc_, mutex->desc_);
}

int DescWrapper::TimedWaitAbs(DescWrapper* mutex,
                              struct nacl_abi_timespec* ts) {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      TimedWaitAbs(desc_,
                   mutex->desc_,
                   ts);
}

int DescWrapper::Signal() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Signal(desc_);
}

int DescWrapper::Broadcast() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Broadcast(desc_);
}

ssize_t DescWrapper::SendMsg(const MsgHeader* dgram, int flags) {
  struct NaClImcTypedMsgHdr header;
  ssize_t ret = -NACL_ABI_ENOMEM;
  nacl_abi_size_t diov_length = dgram->iov_length;
  nacl_abi_size_t ddescv_length = dgram->ndescv_length;
  nacl_abi_size_t i;

  // Initialize to allow simple cleanups.
  header.ndescv = NULL;
  // Allocate and copy IOV.
  if (kSizeTMax / sizeof(NaClImcMsgIoVec) <= diov_length) {
    goto cleanup;
  }
  header.iov = reinterpret_cast<NaClImcMsgIoVec*>(
      calloc(diov_length, sizeof(*(header.iov))));
  if (NULL == header.iov) {
    goto cleanup;
  }
  header.iov_length = diov_length;
  for (i = 0; i < dgram->iov_length; ++i) {
    header.iov[i].base = dgram->iov[i].base;
    header.iov[i].length = dgram->iov[i].length;
  }
  // Allocate and copy the descriptor vector, removing DescWrappers.
  if (kHandleCountMax < dgram->ndescv_length) {
    goto cleanup;
  }
  if (kSizeTMax / sizeof(header.ndescv[0]) <= ddescv_length) {
    goto cleanup;
  }
  header.ndescv = reinterpret_cast<NaClDesc**>(
      calloc(ddescv_length, sizeof(*(header.ndescv))));
  if (NULL == header.iov) {
    goto cleanup;
  }
  header.ndesc_length = ddescv_length;
  for (i = 0; i < dgram->ndescv_length; ++i) {
    header.ndescv[i] = dgram->ndescv[i]->desc_;
  }
  // Send the message.
  ret = NaClImcSendTypedMessage(desc_, &header, flags);

 cleanup:
  free(header.ndescv);
  free(header.iov);
  return ret;
}

ssize_t DescWrapper::RecvMsg(MsgHeader* dgram, int flags) {
  struct NaClImcTypedMsgHdr header;
  ssize_t ret = -NACL_ABI_ENOMEM;
  nacl_abi_size_t diov_length = dgram->iov_length;
  nacl_abi_size_t ddescv_length = dgram->ndescv_length;
  nacl_abi_size_t i;

  // Initialize to allow simple cleanups.
  header.ndescv = NULL;
  for (i = 0; i < dgram->ndescv_length; ++i) {
    dgram->ndescv[i] = NULL;
  }

  // Allocate and copy the IOV.
  if (kSizeTMax / sizeof(NaClImcMsgIoVec) <= diov_length) {
    goto cleanup;
  }
  header.iov = reinterpret_cast<NaClImcMsgIoVec*>(
      calloc(diov_length, sizeof(*(header.iov))));
  if (NULL == header.iov) {
    goto cleanup;
  }
  header.iov_length = diov_length;
  for (i = 0; i < dgram->iov_length; ++i) {
    header.iov[i].base = dgram->iov[i].base;
    header.iov[i].length = dgram->iov[i].length;
  }
  // Allocate and copy the descriptor vector.
  if (kHandleCountMax < dgram->ndescv_length) {
    goto cleanup;
  }
  if (kSizeTMax / sizeof(header.ndescv[0]) <= ddescv_length) {
    goto cleanup;
  }
  header.ndescv = reinterpret_cast<NaClDesc**>(
      calloc(ddescv_length, sizeof(*(header.ndescv))));
  if (NULL == header.ndescv) {
    goto cleanup;
  }
  header.ndesc_length = ddescv_length;
  // Receive the message.
  ret = NaClImcRecvTypedMessage(desc_, &header, flags);
  if (ret < 0) {
    goto cleanup;
  }
  dgram->ndescv_length = header.ndesc_length;
  dgram->flags = header.flags;
  // Copy the descriptors, creating new DescWrappers around them.
  for (i = 0; i < header.ndesc_length; ++i) {
    dgram->ndescv[i] =
        new(std::nothrow) DescWrapper(common_data_, header.ndescv[i]);
    if (NULL == dgram->ndescv[i]) {
      goto cleanup;
    }
  }
  free(header.ndescv);
  free(header.iov);
  return ret;

 cleanup:
  for (i = 0; i < ddescv_length; ++i) {
    delete dgram->ndescv[i];
  }
  free(header.ndescv);
  free(header.iov);
  return ret;
}

DescWrapper* DescWrapper::Connect() {
  struct NaClDesc* connected_desc;
  int rv = reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      ConnectAddr(desc_,
                  &connected_desc);
  if (0 != rv) {
    // Connect failed.
    return NULL;
  }
  DescWrapper* wrapper =
      new(std::nothrow) DescWrapper(common_data_, connected_desc);
  if (NULL == wrapper) {
    NaClDescUnref(connected_desc);
  }
  return wrapper;
}

DescWrapper* DescWrapper::Accept() {
  struct NaClDesc* connected_desc;
  int rv = reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      AcceptConn(desc_, &connected_desc);
  if (0 != rv) {
    // Accept failed.
    return NULL;
  }
  DescWrapper* wrapper =
      new(std::nothrow) DescWrapper(common_data_, connected_desc);
  if (NULL == wrapper) {
    NaClDescUnref(connected_desc);
  }
  return wrapper;
}

int DescWrapper::Post() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      Post(desc_);
}

int DescWrapper::SemWait() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      SemWait(desc_);
}

int DescWrapper::GetValue() {
  return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
      GetValue(desc_);
}

}  // namespace nacl
