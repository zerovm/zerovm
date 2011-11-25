/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, non-platform specific system call helper routines.
 */

#ifndef NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__
#define NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__ 1

#include "native_client/src/include/portability.h"

#include "native_client/src/include/nacl_base.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/trusted/service_runtime/include/sys/time.h"

EXTERN_C_BEGIN

struct NaClAbiNaClImcMsgHdr;
struct NaClApp;
struct NaClAppThread;
struct NaClSocketAddress;
struct NaClDesc;
struct NaClImcMsgHdr;
struct nacl_abi_stat;

int32_t NaClSetBreak(struct NaClAppThread *natp,
                     uintptr_t            new_break);

/*
 * Entering kernel mode from user mde.  Makes thread unkillable
 * because while switched to kernel mode, we may be holding kernel
 * locks and in the middle of mutating protected objects, and killing
 * the thread will leave the service runtime in an inconsistent state.
 * Must invoke the correspondng Leave function, except if the thread
 * is exiting anyway.
 *
 * Not all syscalls need to invoke the Enter function -- those that we
 * know definitely does not grab any locks (even implicitly, in libc
 * routines such as malloc).
 */
void NaClSysCommonThreadSyscallEnter(struct NaClAppThread *natp);

/*
 * Thread is leaving kernel mode and returning to user mode.  Checks
 * if a kill request was made, and if so commit suicide.  Must be
 * invoked if there was a corresponding Enter; not harmful if called
 * without a corresponding Enter.
 */
void NaClSysCommonThreadSyscallLeave(struct NaClAppThread *natp);

int NaClHighResolutionTimerEnabled(void);

int32_t NaClOpenAclCheck(struct NaClApp *nap,
                         char const     *path,
                         int            flags,
                         int            mode);

int32_t NaClStatAclCheck(struct NaClApp *nap,
                         char const     *path);

int32_t NaClCommonSysExit(struct NaClAppThread  *natp,
                          int                   status);

int32_t NaClCommonSysThreadExit(struct NaClAppThread  *natp,
                                int32_t               *stack_flag);

extern int NaClAclBypassChecks;

void NaClInsecurelyBypassAllAclChecks(void);

int32_t NaClCommonSysNameService(struct NaClAppThread *natp,
                                 int32_t              *desc_addr);

int32_t NaClCommonSysDup(struct NaClAppThread *natp,
                         int                  oldfd);

int32_t NaClCommonSysDup2(struct NaClAppThread  *natp,
                          int                   oldfd,
                          int                   newfd);

int32_t NaClCommonSysOpen(struct NaClAppThread  *natp,
                          char                  *pathname,
                          int                   flags,
                          int                   mode);

int32_t NaClCommonSysClose(struct NaClAppThread *natp,
                           int                  d);

int32_t NaClCommonSysRead(struct NaClAppThread  *natp,
                          int                   d,
                          void                  *buf,
                          size_t                count);

int32_t NaClCommonSysWrite(struct NaClAppThread *natp,
                           int                  d,
                           void                 *buf,
                           size_t               count);

int32_t NaClCommonSysLseek(struct NaClAppThread *natp,
                           int                  d,
                           nacl_abi_off_t       *offp,
                           int                  whence);

int32_t NaClCommonSysIoctl(struct NaClAppThread *natp,
                           int                  d,
                           int                  request,
                           void                 *arg);

int32_t NaClCommonSysFstat(struct NaClAppThread *natp,
                           int                  d,
                           struct nacl_abi_stat *nasp);

int32_t NaClCommonSysStat(struct NaClAppThread *natp,
                          const char           *path,
                          struct nacl_abi_stat *nasp);

void NaClCommonUtilUpdateAddrMap(struct NaClApp       *nap,
                                 uintptr_t            sysaddr,
                                 size_t               nbytes,
                                 int                  sysprot,
                                 struct NaClDesc      *backing_desc,
                                 nacl_off64_t         backing_bytes,
                                 nacl_off64_t         offset_bytes,
                                 int                  delete_mem);

/* bool */
int NaClSysCommonAddrRangeContainsExecutablePages_mu(struct NaClApp *nap,
                                                     uintptr_t      usraddr,
                                                     size_t         length);

int32_t NaClCommonSysMmap(struct NaClAppThread  *natp,
                          void                  *start,
                          size_t                length,
                          int                   prot,
                          int                   flags,
                          int                   d,
                          nacl_abi_off_t        *offp);

int32_t NaClCommonSysMmapIntern(struct NaClApp        *nap,
                                void                  *start,
                                size_t                length,
                                int                   prot,
                                int                   flags,
                                int                   d,
                                nacl_abi_off_t        offset);

int32_t NaClSysMmap(struct NaClAppThread  *natp,
                    void                  *start,
                    size_t                length,
                    int                   prot,
                    int                   flags,
                    int                   d,
                    nacl_abi_off_t        *offp);

int32_t NaClSysMunmap(struct NaClAppThread  *natp,
                      void                  *start,
                      size_t                length);

int32_t NaClCommonSysGetdents(struct NaClAppThread  *natp,
                              int                   d,
                              void                  *dirp,
                              size_t                count);

int32_t NaClCommonSysImc_MakeBoundSock(struct NaClAppThread *natp,
                                       int32_t              *sap);

int32_t NaClCommonSysImc_Accept(struct NaClAppThread  *natp,
                                int                   d);

int32_t NaClCommonSysImc_Connect(struct NaClAppThread *natp,
                                 int                  d);

int32_t NaClCommonSysImc_Sendmsg(struct NaClAppThread         *natp,
                                 int                          d,
                                 struct NaClAbiNaClImcMsgHdr  *nanimhp,
                                 int                          flags);

int32_t NaClCommonSysImc_Recvmsg(struct NaClAppThread         *natp,
                                 int                          d,
                                 struct NaClAbiNaClImcMsgHdr  *nanimhp,
                                 int                          flags);

int32_t NaClCommonSysImc_Mem_Obj_Create(struct NaClAppThread  *natp,
                                        size_t                size);

int32_t NaClCommonSysTls_Init(struct NaClAppThread  *natp,
                              void                  *thread_ptr);

int32_t NaClCommonSysThread_Create(struct NaClAppThread *natp,
                                   void                 *eip,
                                   void                 *stack_ptr,
                                   void                 *thread_ptr,
                                   void                 *second_thread_ptr);

int32_t NaClCommonSysTlsGet(struct NaClAppThread *natp);

int32_t NaClCommonSysThread_Nice(struct NaClAppThread *natp,
                                 const int nice);

/* mutex */

int32_t NaClCommonSysMutex_Create(struct NaClAppThread *natp);

int32_t NaClCommonSysMutex_Lock(struct NaClAppThread *natp,
                                int32_t              mutex_handle);

int32_t NaClCommonSysMutex_Unlock(struct NaClAppThread *natp,
                                  int32_t              mutex_handle);

int32_t NaClCommonSysMutex_Trylock(struct NaClAppThread *natp,
                                   int32_t              mutex_handle);

/* condition variable */

int32_t NaClCommonSysCond_Create(struct NaClAppThread *natp);

int32_t NaClCommonSysCond_Wait(struct NaClAppThread *natp,
                               int32_t              cond_handle,
                               int32_t              mutex_handle);

int32_t NaClCommonSysCond_Signal(struct NaClAppThread *natp,
                                 int32_t              cond_handle);

int32_t NaClCommonSysCond_Broadcast(struct NaClAppThread *natp,
                                    int32_t              cond_handle);

int32_t NaClCommonSysCond_Timed_Wait_Rel(struct NaClAppThread     *natp,
                                         int32_t                  cond_handle,
                                         int32_t                  mutex_handle,
                                         struct nacl_abi_timespec *ts);

int32_t NaClCommonSysCond_Timed_Wait_Abs(struct NaClAppThread     *natp,
                                         int32_t                  cond_handle,
                                         int32_t                  mutex_handle,
                                         struct nacl_abi_timespec *ts);

int32_t NaClCommonDescSocketPair(struct NaClDesc      **pair);

int32_t NaClCommonSysImc_SocketPair(struct NaClAppThread *natp,
                                    int32_t volatile     *d_out);
/* Semaphores */
int32_t NaClCommonSysSem_Create(struct NaClAppThread *natp,
                                int32_t              init_value);

int32_t NaClCommonSysSem_Wait(struct NaClAppThread *natp,
                              int32_t              sem_handle);

int32_t NaClCommonSysSem_Post(struct NaClAppThread *natp,
                              int32_t              sem_handle);

int32_t NaClCommonSysSem_Get_Value(struct NaClAppThread *natp,
                                   int32_t              sem_handle);

int32_t NaClCommonSysTest_InfoLeak(struct NaClAppThread *natp);

EXTERN_C_END

#endif  /* NATIVE_CLIENT_SERVICE_RUNTIME_NACL_SYSCALL_COMMON_H__ */
