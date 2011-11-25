// Copyright (c) 2011 The Native Client Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_WRAPPER_H_
#define NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_WRAPPER_H_

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"

namespace nacl {
// Forward declarations.
class DescWrapper;
class DescWrapperCommon;

// We also create a utility class that allows creation of wrappers for the
// NaClDescs.
class DescWrapperFactory {
 public:
  DescWrapperFactory();
  ~DescWrapperFactory();

  // Create a bound socket, socket address pair.
  int MakeBoundSock(DescWrapper* pair[2]);
  // Create a pair of DescWrappers for a connnected (data-only) socket.
  int MakeSocketPair(DescWrapper* pair[2]);
  // Create an IMC socket object.
  DescWrapper* MakeImcSock(NaClHandle handle);
  // Create a shared memory object.
  DescWrapper* MakeShm(size_t size);
  // Create a file descriptor object.
  DescWrapper* MakeFileDesc(int host_os_desc, int mode);
  // Create a DescWrapper from opening a host file.
  DescWrapper* OpenHostFile(const char* fname, int flags, int mode);
  // Create a DescWrapper for the designated invalid descriptor
  DescWrapper* MakeInvalid();

  // Create a DescWrapper from a generic Pepper shared memory descriptor.
  DescWrapper* ImportPepperSharedMemory(intptr_t shm, size_t size);
  // Create a DescWrapper from a Pepper2D shared memory descriptor.
  DescWrapper* ImportPepper2DSharedMemory(intptr_t dib);
  // Create a DescWrapper from a Pepper synchronization object.
  DescWrapper* ImportPepperSync(intptr_t sock);

  // We will doubtless want more specific factory methods.  For now,
  // we provide a wide-open method.
  DescWrapper* MakeGeneric(struct NaClDesc* desc);
  // Same as above but unrefs desc in case of failure
  DescWrapper* MakeGenericCleanup(struct NaClDesc* desc);
  // Utility routine for importing sync socket
  DescWrapper* ImportSyncSocketHandle(NaClHandle handle);
  // Utility routine for importing Linux/Mac (posix) and Windows shared memory.
  DescWrapper* ImportShmHandle(NaClHandle handle, size_t size);
  // Utility routine for importing SysV shared memory.
  DescWrapper* ImportSysvShm(int key, size_t size);

 private:
  // The common data from this instance of the wrapper.
  DescWrapperCommon* common_data_;

  DISALLOW_COPY_AND_ASSIGN(DescWrapperFactory);
};

// A wrapper around NaClDesc to provide slightly higher level abstractions for
// common operations.
class DescWrapper {
  friend class DescWrapperFactory;

 public:
  struct MsgIoVec {
    void*           base;
    nacl_abi_size_t length;
  };

  struct MsgHeader {
    struct MsgIoVec* iov;
    nacl_abi_size_t  iov_length;
    DescWrapper**    ndescv;  // Pointer to array of pointers.
    nacl_abi_size_t  ndescv_length;
    int32_t          flags;
    // flags may contain 0 or any combination of the following.
    static const int32_t kRecvMsgDataTruncated =
        NACL_ABI_RECVMSG_DATA_TRUNCATED;
    static const int32_t kRecvMsgDescTruncated =
        NACL_ABI_RECVMSG_DESC_TRUNCATED;
  };

  ~DescWrapper();

  // Extract a NaClDesc from the wrapper.
  struct NaClDesc* desc() const { return desc_; }

  // Get the type of the wrapped NaClDesc.
  enum NaClDescTypeTag type_tag() const {
    return reinterpret_cast<struct NaClDescVtbl const *>(desc_->base.vtbl)->
        typeTag;
  }

  // We do not replicate the underlying NaClDesc object hierarchy, so there
  // are obviously many more methods than a particular derived class
  // implements.

  // Map a shared memory descriptor.
  // Sets the address to the place mapped to and the size to the rounded result.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Map(void** addr, size_t* size);

  // Unmaps a region of shared memory.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Unmap(void* start_addr, size_t len);

  // Read len bytes into buf.
  // Returns bytes read on success, negative NaCl ABI errno on failure.
  ssize_t Read(void* buf, size_t len);

  // Write len bytes from buf.
  // Returns bytes written on success, negative NaCl ABI errno on failure.
  ssize_t Write(const void* buf, size_t len);

  // Move the file pointer.
  // Returns updated position on success, negative NaCl ABI errno on failure.
  nacl_off64_t Seek(nacl_off64_t offset, int whence);

  // The generic I/O control function.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Ioctl(int request, void* arg);

  // Get descriptor information.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Fstat(struct nacl_abi_stat* statbuf);

  // Close the descriptor.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Close();

  // Read count directory entries into dirp.
  // Returns count on success, negative NaCl ABI errno on failure.
  ssize_t Getdents(void* dirp, size_t count);

  // Lock a mutex.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Lock();

  // TryLock on a mutex.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int TryLock();

  // Unlock a mutex.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Unlock();

  // Wait on a condition variable guarded by the specified mutex.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Wait(DescWrapper* mutex);

  // Timed wait on a condition variable guarded by the specified mutex.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int TimedWaitAbs(DescWrapper* mutex, struct nacl_abi_timespec* ts);

  // Signal a condition variable.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Signal();

  // Broadcast to a condition variable.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Broadcast();

  // Send a message.
  // Returns bytes sent on success, negative NaCl ABI errno on failure.
  ssize_t SendMsg(const MsgHeader* dgram, int flags);

  // Receive a message.
  // Returns bytes received on success, negative NaCl ABI errno on failure.
  ssize_t RecvMsg(MsgHeader* dgram, int flags);

  // Connect to a socket address.
  // Returns a valid DescWrapper on success, NULL on failure.
  DescWrapper* Connect();

  // Accept connection on a bound socket.
  // Returns a valid DescWrapper on success, NULL on failure.
  DescWrapper* Accept();

  // Post on a semaphore.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int Post();

  // Wait on a semaphore.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int SemWait();

  // Get a semaphore's value.
  // Returns zero on success, negative NaCl ABI errno on failure.
  int GetValue();

 private:
  DescWrapper(DescWrapperCommon* common_data, struct NaClDesc* desc);

  DescWrapperCommon* common_data_;
  struct NaClDesc* desc_;

  DISALLOW_COPY_AND_ASSIGN(DescWrapper);
};

}  // namespace nacl

#endif  // NATIVE_CLIENT_SRC_TRUSTED_DESC_NACL_DESC_WRAPPER_H_
