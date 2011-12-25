/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#include "include/portability.h"
#include "include/nacl_platform.h"
#include "include/nacl_macros.h"

#include "src/platform/nacl_check.h"
#include "src/platform/nacl_log.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/platform/nacl_time.h"

#include "src/desc/nacl_desc_base.h"
#include "src/desc/nacl_desc_conn_cap.h"
#include "src/desc/nacl_desc_imc.h"
#include "src/desc/nacl_desc_imc_bound_desc.h"
#include "src/desc/nacl_desc_imc_shm.h"
#include "src/desc/nacl_desc_io.h"

#include "src/service_runtime/nacl_app_thread.h"
#include "src/service_runtime/nacl_config.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_memory_object.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/nacl_syscall_common.h"
#include "src/service_runtime/nacl_text.h"
#include "src/service_runtime/sel_util.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/sel_memory.h"

#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/include/bits/nacl_syscalls.h"
#include "src/service_runtime/include/machine/_types.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/fcntl.h"
#include "src/service_runtime/include/sys/nacl_imc_api.h"
#include "src/service_runtime/include/sys/stat.h"
#include "src/service_runtime/include/sys/time.h"
#include "src/service_runtime/include/sys/unistd.h"

#include "src/service_runtime/linux/nacl_syscall_inl.h"
#include "src/manifest/trap.h"
#include "src/manifest/manifest_setup.h"
#include "api/zvm.h"

struct NaClSyscallTableEntry nacl_syscall[NACL_MAX_SYSCALLS] = {{0}};


static int32_t NotImplementedDecoder(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
  return -NACL_ABI_ENOSYS;
}

static void NaClAddSyscall(int num, int32_t (*fn)(struct NaClAppThread *)) {
  if (nacl_syscall[num].handler != &NotImplementedDecoder) {
    NaClLog(LOG_FATAL, "Duplicate syscall number %d\n", num);
  }
  nacl_syscall[num].handler = fn;
}

/* ====================================================================== */

int32_t NaClSysNull(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
  return 0;
}

int32_t NaClSysNameService(struct NaClAppThread *natp,
                           int                  *desc_in_out) {
  return NaClCommonSysNameService(natp, desc_in_out);
}

int32_t NaClSysDup(struct NaClAppThread *natp,
                   int                  oldfd) {
  return NaClCommonSysDup(natp, oldfd);
}

int32_t NaClSysDup2(struct NaClAppThread  *natp,
                    int                   oldfd,
                    int                   newfd) {
  return NaClCommonSysDup2(natp, oldfd, newfd);
}

int32_t NaClSysOpen(struct NaClAppThread  *natp,
                    char                  *pathname,
                    int                   flags,
                    int                   mode) {
  return NaClCommonSysOpen(natp, pathname, flags, mode);
}

int32_t NaClSysClose(struct NaClAppThread *natp,
                     int                  d) {
  return NaClCommonSysClose(natp, d);
}

int32_t NaClSysRead(struct NaClAppThread  *natp,
                    int                   d,
                    void                  *buf,
                    size_t                count) {
  return NaClCommonSysRead(natp, d, buf, count);
}

int32_t NaClSysWrite(struct NaClApp *nap,
                     int                  d,
                     void                 *buf,
                     size_t               count) {
  return NaClCommonSysWrite(nap, d, buf, count);
}

/* Warning: sizeof(nacl_abi_off_t)!=sizeof(off_t) on OSX */
int32_t NaClSysLseek(struct NaClAppThread *natp,
                     int                  d,
                     nacl_abi_off_t       *offp,
                     int                  whence) {
  return NaClCommonSysLseek(natp, d, offp, whence);
}

int32_t NaClSysIoctl(struct NaClAppThread *natp,
                     int                  d,
                     int                  request,
                     void                 *arg) {
  return NaClCommonSysIoctl(natp, d, request, arg);
}

int32_t NaClSysFstat(struct NaClApp *nap, int d,
                     struct nacl_abi_stat *nasp) {
  return NaClCommonSysFstat(nap, d, nasp);
}

int32_t NaClSysStat(struct NaClAppThread *natp,
                    const char           *path,
                    struct nacl_abi_stat *nasp) {
  return NaClCommonSysStat(natp, path, nasp);
}

int32_t NaClSysGetdents(struct NaClAppThread  *natp,
                        int                   d,
                        void                  *buf,
                        size_t                count) {
  return NaClCommonSysGetdents(natp, d, buf, count);
}

int32_t NaClSysSysbrk(struct NaClApp  *nap,
                      void            *new_break) {
  return NaClSetBreak(nap, (uintptr_t) new_break);
}

int32_t NaClSysMmap(struct NaClApp  *nap,
                    void                  *start,
                    size_t                length,
                    int                   prot,
                    int                   flags,
                    int                   d,
                    nacl_abi_off_t        *offp) {
  return NaClCommonSysMmap(nap, start, length, prot, flags, d, offp);
}

int32_t NaClSysMunmap(struct NaClAppThread  *natp,
                      void                  *start,
                      size_t                length) {
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sysaddr;
  int       holding_app_lock = 0;
  size_t    alloc_rounded_length;

  NaClLog(3, "Entered NaClSysMunmap(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS")\n",
          (uintptr_t) natp, (uintptr_t) start, length);

  NaClSysCommonThreadSyscallEnter(natp);

  if (!NaClIsAllocPageMultiple((uintptr_t) start)) {
    NaClLog(4, "start addr not allocation multiple\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  if (0 == length) {
    /*
     * linux mmap of zero length yields a failure, but osx does not, leading
     * to a NaClVmmapUpdate of zero pages, which should not occur.
     */
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  alloc_rounded_length = NaClRoundAllocPage(length);
  if (alloc_rounded_length != length) {
    length = alloc_rounded_length;
    NaClLog(LOG_WARNING,
            "munmap: rounded length to 0x%"NACL_PRIxS"\n", length);
  }
  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) start, length);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "region not user addresses\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClXMutexLock(&natp->nap->mu);

  while (0 != natp->nap->threads_launching) {
    NaClXCondVarWait(&natp->nap->cv, &natp->nap->mu);
  }
  natp->nap->vm_hole_may_exist = 1;

  holding_app_lock = 1;
  /*
   * NB: windows (or generic) version would use Munmap virtual
   * function from the backing NaClDesc object obtained by iterating
   * through the address map for the region, and those Munmap virtual
   * functions may return -NACL_ABI_E_MOVE_ADDRESS_SPACE.
   *
   * We should hold the application lock while doing this iteration
   * and unmapping, so that the address space is consistent for other
   * threads.
   */

  /*
   * User should be unable to unmap any executable pages.  We check here.
   */
  if (NaClSysCommonAddrRangeContainsExecutablePages_mu(natp->nap,
                                                       (uintptr_t) start,
                                                       length)) {
    NaClLog(2, "NaClSysMunmap: region contains executable pages\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  /*
   * Overwrite current mapping with inaccessible, anonymous
   * zero-filled pages, which should be copy-on-write and thus
   * relatively cheap.  Do not open up an address space hole.
   */
  NaClLog(4,
          ("NaClSysMunmap: mmap(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS","
           " 0x%x, 0x%x, -1, 0)\n"),
          sysaddr, length, PROT_NONE,
          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED);
  if (MAP_FAILED == mmap((void *) sysaddr,
                         length,
                         PROT_NONE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                         -1,
                         (off_t) 0)) {
    NaClLog(4, "mmap to put in anonymous memory failed, errno = %d\n", errno);
    retval = -NaClXlateErrno(errno);
    goto cleanup;
  }
  NaClVmmapUpdate(&natp->nap->mem_map,
                  (NaClSysToUser(natp->nap, (uintptr_t) sysaddr)
                   >> NACL_PAGESHIFT),
                  length >> NACL_PAGESHIFT,
                  0,  /* prot */
                  (struct NaClMemObj *) NULL,
                  1);  /* Delete mapping */
  retval = 0;
cleanup:
  if (holding_app_lock) {
    natp->nap->vm_hole_may_exist = 0;
    NaClXCondVarBroadcast(&natp->nap->cv);
    NaClXMutexUnlock(&natp->nap->mu);
  }
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClSysExit(struct NaClApp *nap, int status)
{
  return NaClCommonSysExit(nap, status);
}

int32_t NaClSysGetpid(struct NaClAppThread *natp) {
  int32_t pid;

  NaClSysCommonThreadSyscallEnter(natp);

  pid = getpid();  /* TODO(bsy): obfuscate? */
  NaClLog(4, "NaClSysGetpid: returning %d\n", pid);

  NaClSysCommonThreadSyscallLeave(natp);
  return pid;
}

int32_t NaClSysThread_Exit(struct NaClAppThread  *natp,
                           int32_t               *stack_flag) {
  return NaClCommonSysThreadExit(natp, stack_flag);
}

int32_t NaClSysGetTimeOfDay(struct NaClAppThread      *natp,
                            struct nacl_abi_timeval   *tv,
                            struct nacl_abi_timezone  *tz) {
  uintptr_t               sysaddr;
  int                     retval;
  struct nacl_abi_timeval now;

  UNREFERENCED_PARAMETER(tz);

  NaClLog(3,
          ("Entered NaClSysGetTimeOfDay(%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) tv, (uintptr_t) tz);
  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) tv, sizeof tv);

  /*
   * tz is not supported in linux, nor is it supported by glibc, since
   * tzset(3) and the zoneinfo file should be used instead.
   *
   * TODO(bsy) Do we make the zoneinfo directory available to
   * applications?
   */

  if (kNaClBadAddress == sysaddr) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  retval = NaClGetTimeOfDay(&now);
  if (0 == retval) {
    /*
     * To make it harder to distinguish Linux platforms from Windows,
     * coarsen the time to the same level we get on Windows -
     * milliseconds, unless in "debug" mode.
     */
    if (!NaClHighResolutionTimerEnabled()) {
      now.nacl_abi_tv_usec = (now.nacl_abi_tv_usec / 1000) * 1000;
    }
    CHECK(now.nacl_abi_tv_usec >= 0);
    CHECK(now.nacl_abi_tv_usec < NACL_MICROS_PER_UNIT);
    *(struct nacl_abi_timeval *) sysaddr = now;
  }
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

/*
 * TODO(bsy): REMOVE THIS AND PROVIDE GETRUSAGE.  This is normally
 * not a syscall; instead, it is a library routine on top of
 * getrusage, with appropriate clock tick translation.
 */
int32_t NaClSysClock(struct NaClAppThread *natp) {
  int32_t retval;

  NaClLog(3,
          ("Entered NaClSysClock(%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  NaClSysCommonThreadSyscallEnter(natp);
  retval = clock();
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClSysNanosleep(struct NaClAppThread     *natp,
                         struct nacl_abi_timespec *req,
                         struct nacl_abi_timespec *rem) {
  uintptr_t                 sys_req;
  uintptr_t                 sys_rem;
  struct nacl_abi_timespec  t_sleep;
  struct nacl_abi_timespec  t_rem;
  struct nacl_abi_timespec  *remptr;
  int                       retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClSysNanosleep(0x%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR"x)\n"),
          (uintptr_t) natp, (uintptr_t) req, (uintptr_t) rem);

  NaClSysCommonThreadSyscallEnter(natp);

  sys_req = NaClUserToSysAddrRange(natp->nap, (uintptr_t) req, sizeof *req);
  if (kNaClBadAddress == sys_req) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  if (NULL == rem) {
    sys_rem = 0;
  } else {
    sys_rem = NaClUserToSysAddrRange(natp->nap, (uintptr_t) rem, sizeof *rem);
    if (kNaClBadAddress == sys_rem) {
      retval = -NACL_ABI_EFAULT;
      goto cleanup;
    }
  }
  /*
   * post-condition: if sys_rem is non-NULL, it's safe to write to
   * (modulo thread races) and the user code wants the remaining time
   * written there.
   */

  NaClLog(4, " copying timespec from %08"NACL_PRIxPTR"x\n", sys_req);
  /* copy once */
  t_sleep = *(struct nacl_abi_timespec *) sys_req;

  remptr = (0 == sys_rem) ? NULL : &t_rem;
  /* NULL != remptr \equiv NULL != rem */

  /*
   * We assume that we do not need to normalize the time request values.
   *
   * If bogus values can cause the underlying OS to get into trouble,
   * then we need more checking here.
   */
  NaClLog(4, "NaClSysNanosleep(time = %"NACL_PRId64".%09"NACL_PRId64" S)\n",
          (int64_t) t_sleep.tv_sec, (int64_t) t_sleep.tv_nsec);
  retval = NaClNanosleep(&t_sleep, remptr);
  NaClLog(4, "NaClNanosleep returned %d\n", retval);

  if (-EINTR == retval && NULL != rem) {
    /* definitely different types, and shape may actually differ too. */
    rem = (struct nacl_abi_timespec *) sys_rem;
    rem->tv_sec = remptr->tv_sec;
    rem->tv_nsec = remptr->tv_nsec;
  }

cleanup:
  NaClLog(4, "nanosleep done.\n");
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClSysImc_MakeBoundSock(struct NaClAppThread *natp,
                                 int32_t              *sap) {
  return NaClCommonSysImc_MakeBoundSock(natp, sap);
}

int32_t NaClSysImc_Accept(struct NaClAppThread  *natp,
                          int                   d) {
  return NaClCommonSysImc_Accept(natp, d);
}

int32_t NaClSysImc_Connect(struct NaClAppThread *natp,
                           int                  d) {
  return NaClCommonSysImc_Connect(natp, d);
}

int32_t NaClSysImc_Sendmsg(struct NaClAppThread         *natp,
                           int                          d,
                           struct NaClAbiNaClImcMsgHdr  *nanimhp,
                           int                          flags) {
  return NaClCommonSysImc_Sendmsg(natp, d, nanimhp, flags);
}

int32_t NaClSysImc_Recvmsg(struct NaClAppThread         *natp,
                           int                          d,
                           struct NaClAbiNaClImcMsgHdr  *nimhp,
                           int                          flags) {
  return NaClCommonSysImc_Recvmsg(natp, d, nimhp, flags);
}

int32_t NaClSysImc_Mem_Obj_Create(struct NaClAppThread  *natp,
                                  nacl_abi_size_t       size) {
  return NaClCommonSysImc_Mem_Obj_Create(natp, size);
}

int32_t NaClSysTls_Init(struct NaClApp *nap, void *thread_ptr)
{
  return NaClCommonSysTls_Init(nap, thread_ptr);
}

int32_t NaClSysThread_Create(struct NaClAppThread *natp,
                             void                 *prog_ctr,
                             void                 *stack_ptr,
                             void                 *thread_ptr,
                             void                 *second_thread_ptr) {
  return NaClCommonSysThread_Create(natp, prog_ctr, stack_ptr, thread_ptr,
                                    second_thread_ptr);
}

int32_t NaClSysTls_Get(struct NaClApp *nap) {
  return NaClCommonSysTlsGet(nap);
}

int32_t NaClSysThread_Nice(struct NaClAppThread *natp, const int nice) {
  return NaClCommonSysThread_Nice(natp, nice);
}

/* mutex */

int32_t NaClSysMutex_Create(struct NaClApp *nap) {
  return NaClCommonSysMutex_Create(nap);
}

int32_t NaClSysMutex_Lock(struct NaClAppThread *natp,
                          int32_t              mutex_handle) {
  return NaClCommonSysMutex_Lock(natp, mutex_handle);
}

int32_t NaClSysMutex_Unlock(struct NaClAppThread *natp,
                            int32_t              mutex_handle) {
  return NaClCommonSysMutex_Unlock(natp, mutex_handle);
}

int32_t NaClSysMutex_Trylock(struct NaClAppThread *natp,
                             int32_t              mutex_handle) {
  return NaClCommonSysMutex_Trylock(natp, mutex_handle);
}


/* condition variable */

int32_t NaClSysCond_Create(struct NaClAppThread *natp) {
  return NaClCommonSysCond_Create(natp);
}

int32_t NaClSysCond_Wait(struct NaClAppThread *natp,
                         int32_t              cond_handle,
                         int32_t              mutex_handle) {
  return NaClCommonSysCond_Wait(natp, cond_handle, mutex_handle);
}

int32_t NaClSysCond_Signal(struct NaClAppThread *natp,
                           int32_t              cond_handle) {
  return NaClCommonSysCond_Signal(natp, cond_handle);
}

int32_t NaClSysCond_Broadcast(struct NaClAppThread *natp,
                              int32_t              cond_handle) {
  return NaClCommonSysCond_Broadcast(natp, cond_handle);
}

int32_t NaClSysCond_Timed_Wait_Abs(struct NaClAppThread     *natp,
                                   int32_t                  cond_handle,
                                   int32_t                  mutex_handle,
                                   struct nacl_abi_timespec *ts) {
  return NaClCommonSysCond_Timed_Wait_Abs(natp,
                                          cond_handle,
                                          mutex_handle,
                                          ts);
}

int32_t NaClSysImc_SocketPair(struct NaClAppThread  *natp,
                              int32_t               *d_out) {
  return NaClCommonSysImc_SocketPair(natp, d_out);
}
int32_t NaClSysSem_Create(struct NaClAppThread *natp,
                          int32_t              init_value) {
  return NaClCommonSysSem_Create(natp, init_value);
}

int32_t NaClSysSem_Wait(struct NaClAppThread *natp,
                        int32_t              sem_handle) {
  return NaClCommonSysSem_Wait(natp, sem_handle);
}

int32_t NaClSysSem_Post(struct NaClAppThread *natp,
                        int32_t              sem_handle) {
  return NaClCommonSysSem_Post(natp, sem_handle);
}

int32_t NaClSysSem_Get_Value(struct NaClAppThread *natp,
                             int32_t              sem_handle) {
  return NaClCommonSysSem_Get_Value(natp, sem_handle);
}

int32_t NaClSysSched_Yield(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
  sched_yield();
  return 0;
}

int32_t NaClSysSysconf(struct NaClAppThread *natp,
                       int32_t name,
                       int32_t *result) {
  int32_t         retval = -NACL_ABI_EINVAL;
  static int32_t  number_of_workers = -1;
  uintptr_t       sysaddr;

  NaClLog(3,
          ("Entered NaClSysSysconf(%08"NACL_PRIxPTR
           "x, %d, 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, name, (uintptr_t) result);

  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) result,
                                   sizeof(*result));
  if (kNaClBadAddress == sysaddr) {
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  switch (name) {
#ifdef _SC_NPROCESSORS_ONLN
    case NACL_ABI__SC_NPROCESSORS_ONLN: {
      if (-1 == number_of_workers) {
        number_of_workers = sysconf(_SC_NPROCESSORS_ONLN);
      }
      if (-1 == number_of_workers) {
        /* failed to get the number of processors */
        retval = -NACL_ABI_EINVAL;
        goto cleanup;
      }
      *(int32_t*)sysaddr = number_of_workers;
      break;
    }
#endif
    case NACL_ABI__SC_SENDMSG_MAX_SIZE: {
      /* TODO(sehr,bsy): this value needs to be determined at run time. */
      const int32_t kImcSendMsgMaxSize = 1 << 16;
      *(int32_t*)sysaddr = kImcSendMsgMaxSize;
      break;
    }
    default:
      retval = -NACL_ABI_EINVAL;
      goto cleanup;
  }
  retval = 0;
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClSysDyncode_Create(struct NaClAppThread *natp,
                            uint32_t             dest,
                            uint32_t             src,
                            uint32_t             size) {
  return NaClTextSysDyncode_Create(natp, dest, src, size);
}

int32_t NaClSysDyncode_Modify(struct NaClAppThread *natp,
                            uint32_t             dest,
                            uint32_t             src,
                            uint32_t             size) {
  return NaClTextSysDyncode_Modify(natp, dest, src, size);
}

int32_t NaClSysDyncode_Delete(struct NaClAppThread *natp,
                            uint32_t             dest,
                            uint32_t             size) {
  return NaClTextSysDyncode_Delete(natp, dest, size);
}

/*
 * Note that this is duplicated in win/nacl_syscall_impl.c but is
 * too trivial to share.
 * TODO(mseaborn): We could reduce the duplication if
 * nacl_syscall_handlers_gen2.py did not scrape the OS-specific files.
 */
int32_t NaClSysSecond_Tls_Set(struct NaClAppThread *natp,
                              uint32_t             new_value) {
  natp->tls2 = new_value;
  return 0;
}

int32_t NaClSysSecond_Tls_Get(struct NaClAppThread *natp) {
  return natp->tls2;
}

int32_t NaClSysTest_InfoLeak(struct NaClAppThread *natp) {
  return NaClCommonSysTest_InfoLeak(natp);
}

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * Automatically generated code.  See nacl_syscall_handlers_gen.py
 *
 * NaCl Server Runtime Service Call abstractions
 */

/* this function was automagically generated */
static int32_t NaClSysNullDecoder(struct NaClAppThread *natp) {
  return NaClSysNull(natp);
}

/* this function was automagically generated */
static int32_t NaClSysNameServiceDecoder(struct NaClAppThread *natp) {
  struct NaClSysNameServiceArgs {
    uint32_t desc_in_out;
  } p = *(struct NaClSysNameServiceArgs *) natp->syscall_args;

  return NaClSysNameService(natp, (int *) (uintptr_t) p.desc_in_out);
}

/* this function was automagically generated */
static int32_t NaClSysDupDecoder(struct NaClAppThread *natp) {
  struct NaClSysDupArgs {
    uint32_t oldfd;
  } p = *(struct NaClSysDupArgs *) natp->syscall_args;

  return NaClSysDup(natp, (int)  p.oldfd);
}

/* this function was automagically generated */
static int32_t NaClSysDup2Decoder(struct NaClAppThread *natp) {
  struct NaClSysDup2Args {
    uint32_t oldfd;
    uint32_t newfd;
  } p = *(struct NaClSysDup2Args *) natp->syscall_args;

  return NaClSysDup2(natp, (int)  p.oldfd, (int)  p.newfd);
}

/* this function was automagically generated */
static int32_t NaClSysOpenDecoder(struct NaClAppThread *natp) {
  struct NaClSysOpenArgs {
    uint32_t pathname;
    uint32_t flags;
    uint32_t mode;
  } p = *(struct NaClSysOpenArgs *) natp->syscall_args;

  return NaClSysOpen(natp, (char *) (uintptr_t) p.pathname, (int)  p.flags, (int)  p.mode);
}

/* this function was automagically generated */
static int32_t NaClSysCloseDecoder(struct NaClAppThread *natp) {
  struct NaClSysCloseArgs {
    uint32_t d;
  } p = *(struct NaClSysCloseArgs *) natp->syscall_args;

  return NaClSysClose(natp, (int)  p.d);
}

/* this function was automagically generated */
static int32_t NaClSysReadDecoder(struct NaClAppThread *natp) {
  struct NaClSysReadArgs {
    uint32_t d;
    uint32_t buf;
    uint32_t count;
  } p = *(struct NaClSysReadArgs *) natp->syscall_args;

  return NaClSysRead(natp, (int)  p.d, (void *) (uintptr_t) p.buf, (size_t)  p.count);
}

/* this function was automagically generated */
static int32_t NaClSysWriteDecoder(struct NaClApp *nap) {
  struct NaClSysWriteArgs {
    uint32_t d;
    uint32_t buf;
    uint32_t count;
  } p = *(struct NaClSysWriteArgs *) nap->syscall_args;

  return NaClSysWrite(nap, (int)  p.d, (void *) (uintptr_t) p.buf, (size_t)  p.count);
}

/* this function was automagically generated */
static int32_t NaClSysLseekDecoder(struct NaClAppThread *natp) {
  struct NaClSysLseekArgs {
    uint32_t d;
    uint32_t offp;
    uint32_t whence;
  } p = *(struct NaClSysLseekArgs *) natp->syscall_args;

  return NaClSysLseek(natp, (int)  p.d, (nacl_abi_off_t *) (uintptr_t) p.offp, (int)  p.whence);
}

/* this function was automagically generated */
static int32_t NaClSysIoctlDecoder(struct NaClAppThread *natp) {
  struct NaClSysIoctlArgs {
    uint32_t d;
    uint32_t request;
    uint32_t arg;
  } p = *(struct NaClSysIoctlArgs *) natp->syscall_args;

  return NaClSysIoctl(natp, (int)  p.d, (int)  p.request, (void *) (uintptr_t) p.arg);
}

/* this function was automagically generated */
static int32_t NaClSysFstatDecoder(struct NaClApp *nap) {
  struct NaClSysFstatArgs {
    uint32_t d;
    uint32_t nasp;
  } p = *(struct NaClSysFstatArgs *) nap->syscall_args;

  return NaClSysFstat(nap, (int)  p.d, (struct nacl_abi_stat *) (uintptr_t) p.nasp);
}

/* this function was automagically generated */
static int32_t NaClSysStatDecoder(struct NaClAppThread *natp) {
  struct NaClSysStatArgs {
    uint32_t path;
    uint32_t nasp;
  } p = *(struct NaClSysStatArgs *) natp->syscall_args;

  return NaClSysStat(natp, (const char *) (uintptr_t) p.path, (struct nacl_abi_stat *) (uintptr_t) p.nasp);
}

/* this function was automagically generated */
static int32_t NaClSysGetdentsDecoder(struct NaClAppThread *natp) {
  struct NaClSysGetdentsArgs {
    uint32_t d;
    uint32_t buf;
    uint32_t count;
  } p = *(struct NaClSysGetdentsArgs *) natp->syscall_args;

  return NaClSysGetdents(natp, (int)  p.d, (void *) (uintptr_t) p.buf, (size_t)  p.count);
}

/* this function was automagically generated */
static int32_t NaClSysSysbrkDecoder(struct NaClApp *nap) {
  struct NaClSysSysbrkArgs {
    uint32_t new_break;
  } p = *(struct NaClSysSysbrkArgs *) nap->syscall_args;

  return NaClSysSysbrk(nap, (void *) (uintptr_t) p.new_break);
}

/* this function was automagically generated */
static int32_t NaClSysMmapDecoder(struct NaClApp *nap) {
  struct NaClSysMmapArgs {
    uint32_t start;
    uint32_t length;
    uint32_t prot;
    uint32_t flags;
    uint32_t d;
    uint32_t offp;
  } p = *(struct NaClSysMmapArgs *) nap->syscall_args;

  // ### temporary disabled
  /* d'b: disable user malloc() */
////  return NACL_ABI_ENOMEM;
//  return natp->nap->manifest->user_setup->heap_ptr;
  /* d'b end */

  return NaClSysMmap(nap, (void *) (uintptr_t) p.start, (size_t)  p.length, (int)  p.prot, (int)  p.flags, (int)  p.d, (nacl_abi_off_t *) (uintptr_t) p.offp);
}

/* this function was automagically generated */
static int32_t NaClSysMunmapDecoder(struct NaClAppThread *natp) {
  struct NaClSysMunmapArgs {
    uint32_t start;
    uint32_t length;
  } p = *(struct NaClSysMunmapArgs *) natp->syscall_args;

  // ### temporary disabled
  /* d'b: disable user free() */
//  return NACL_ABI_EFAULT;
  /* d'b end */

  return NaClSysMunmap(natp, (void *) (uintptr_t) p.start, (size_t)  p.length);
}

/* this function was automagically generated */
static int32_t NaClSysExitDecoder(struct NaClApp *nap) {
  struct NaClSysExitArgs {
    uint32_t status;
  } p = *(struct NaClSysExitArgs *) nap->syscall_args;

  return NaClSysExit(nap, (int)  p.status);
}

/* this function was automagically generated */
static int32_t NaClSysGetpidDecoder(struct NaClAppThread *natp) {
  return NaClSysGetpid(natp);
}

/* this function was automagically generated */
static int32_t NaClSysThread_ExitDecoder(struct NaClAppThread *natp) {
  struct NaClSysThread_ExitArgs {
    uint32_t stack_flag;
  } p = *(struct NaClSysThread_ExitArgs *) natp->syscall_args;

  return NaClSysThread_Exit(natp, (int32_t *) (uintptr_t) p.stack_flag);
}

/* this function was automagically generated */
static int32_t NaClSysGetTimeOfDayDecoder(struct NaClAppThread *natp) {
  struct NaClSysGetTimeOfDayArgs {
    uint32_t tv;
    uint32_t tz;
  } p = *(struct NaClSysGetTimeOfDayArgs *) natp->syscall_args;

  return NaClSysGetTimeOfDay(natp, (struct nacl_abi_timeval *) (uintptr_t) p.tv, (struct nacl_abi_timezone *) (uintptr_t) p.tz);
}

/* this function was automagically generated */
static int32_t NaClSysClockDecoder(struct NaClAppThread *natp) {
  return NaClSysClock(natp);
}

/* this function was automagically generated */
static int32_t NaClSysNanosleepDecoder(struct NaClAppThread *natp) {
  struct NaClSysNanosleepArgs {
    uint32_t req;
    uint32_t rem;
  } p = *(struct NaClSysNanosleepArgs *) natp->syscall_args;

  return NaClSysNanosleep(natp, (struct nacl_abi_timespec *) (uintptr_t) p.req, (struct nacl_abi_timespec *) (uintptr_t) p.rem);
}

/* this function was automagically generated */
static int32_t NaClSysImc_MakeBoundSockDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_MakeBoundSockArgs {
    uint32_t sap;
  } p = *(struct NaClSysImc_MakeBoundSockArgs *) natp->syscall_args;

  return NaClSysImc_MakeBoundSock(natp, (int32_t *) (uintptr_t) p.sap);
}

/* this function was automagically generated */
static int32_t NaClSysImc_AcceptDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_AcceptArgs {
    uint32_t d;
  } p = *(struct NaClSysImc_AcceptArgs *) natp->syscall_args;

  return NaClSysImc_Accept(natp, (int)  p.d);
}

/* this function was automagically generated */
static int32_t NaClSysImc_ConnectDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_ConnectArgs {
    uint32_t d;
  } p = *(struct NaClSysImc_ConnectArgs *) natp->syscall_args;

  return NaClSysImc_Connect(natp, (int)  p.d);
}

/* this function was automagically generated */
static int32_t NaClSysImc_SendmsgDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_SendmsgArgs {
    uint32_t d;
    uint32_t nanimhp;
    uint32_t flags;
  } p = *(struct NaClSysImc_SendmsgArgs *) natp->syscall_args;

  return NaClSysImc_Sendmsg(natp, (int)  p.d, (struct NaClAbiNaClImcMsgHdr *) (uintptr_t) p.nanimhp, (int)  p.flags);
}

/* this function was automagically generated */
static int32_t NaClSysImc_RecvmsgDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_RecvmsgArgs {
    uint32_t d;
    uint32_t nimhp;
    uint32_t flags;
  } p = *(struct NaClSysImc_RecvmsgArgs *) natp->syscall_args;

  return NaClSysImc_Recvmsg(natp, (int)  p.d, (struct NaClAbiNaClImcMsgHdr *) (uintptr_t) p.nimhp, (int)  p.flags);
}

/* this function was automagically generated */
static int32_t NaClSysImc_Mem_Obj_CreateDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_Mem_Obj_CreateArgs {
    uint32_t size;
  } p = *(struct NaClSysImc_Mem_Obj_CreateArgs *) natp->syscall_args;

  return NaClSysImc_Mem_Obj_Create(natp, (nacl_abi_size_t)  p.size);
}

/* this function was automagically generated */
static int32_t NaClSysTls_InitDecoder(struct NaClApp *nap)
{
    struct NaClSysTls_InitArgs {
    uint32_t thread_ptr;
  } p = *(struct NaClSysTls_InitArgs *) nap->syscall_args;

  return NaClSysTls_Init(nap, (void *) (uintptr_t) p.thread_ptr);
}

/* this function was automagically generated */
static int32_t NaClSysThread_CreateDecoder(struct NaClAppThread *natp) {
  struct NaClSysThread_CreateArgs {
    uint32_t prog_ctr;
    uint32_t stack_ptr;
    uint32_t thread_ptr;
    uint32_t second_thread_ptr;
  } p = *(struct NaClSysThread_CreateArgs *) natp->syscall_args;

  return NaClSysThread_Create(natp, (void *) (uintptr_t) p.prog_ctr, (void *) (uintptr_t) p.stack_ptr, (void *) (uintptr_t) p.thread_ptr, (void *) (uintptr_t) p.second_thread_ptr);
}

/* this function was automagically generated */
static int32_t NaClSysTls_GetDecoder(struct NaClApp *nap)
{
  return NaClSysTls_Get(nap);
}

/* this function was automagically generated */
static int32_t NaClSysThread_NiceDecoder(struct NaClAppThread *natp) {
  struct NaClSysThread_NiceArgs {
    uint32_t nice;
  } p = *(struct NaClSysThread_NiceArgs *) natp->syscall_args;

  return NaClSysThread_Nice(natp, (const int)  p.nice);
}

/* this function was automagically generated */
static int32_t NaClSysMutex_CreateDecoder(struct NaClApp *nap)
{
  return NaClSysMutex_Create(nap);
}

/* this function was automagically generated */
static int32_t NaClSysMutex_LockDecoder(struct NaClAppThread *natp) {
  struct NaClSysMutex_LockArgs {
    uint32_t mutex_handle;
  } p = *(struct NaClSysMutex_LockArgs *) natp->syscall_args;

  return NaClSysMutex_Lock(natp, (int32_t)  p.mutex_handle);
}

/* this function was automagically generated */
static int32_t NaClSysMutex_UnlockDecoder(struct NaClAppThread *natp) {
  struct NaClSysMutex_UnlockArgs {
    uint32_t mutex_handle;
  } p = *(struct NaClSysMutex_UnlockArgs *) natp->syscall_args;

  return NaClSysMutex_Unlock(natp, (int32_t)  p.mutex_handle);
}

/* this function was automagically generated */
static int32_t NaClSysMutex_TrylockDecoder(struct NaClAppThread *natp) {
  struct NaClSysMutex_TrylockArgs {
    uint32_t mutex_handle;
  } p = *(struct NaClSysMutex_TrylockArgs *) natp->syscall_args;

  return NaClSysMutex_Trylock(natp, (int32_t)  p.mutex_handle);
}

/* this function was automagically generated */
static int32_t NaClSysCond_CreateDecoder(struct NaClAppThread *natp) {
  return NaClSysCond_Create(natp);
}

/* this function was automagically generated */
static int32_t NaClSysCond_WaitDecoder(struct NaClAppThread *natp) {
  struct NaClSysCond_WaitArgs {
    uint32_t cond_handle;
    uint32_t mutex_handle;
  } p = *(struct NaClSysCond_WaitArgs *) natp->syscall_args;

  return NaClSysCond_Wait(natp, (int32_t)  p.cond_handle, (int32_t)  p.mutex_handle);
}

/* this function was automagically generated */
static int32_t NaClSysCond_SignalDecoder(struct NaClAppThread *natp) {
  struct NaClSysCond_SignalArgs {
    uint32_t cond_handle;
  } p = *(struct NaClSysCond_SignalArgs *) natp->syscall_args;

  return NaClSysCond_Signal(natp, (int32_t)  p.cond_handle);
}

/* this function was automagically generated */
static int32_t NaClSysCond_BroadcastDecoder(struct NaClAppThread *natp) {
  struct NaClSysCond_BroadcastArgs {
    uint32_t cond_handle;
  } p = *(struct NaClSysCond_BroadcastArgs *) natp->syscall_args;

  return NaClSysCond_Broadcast(natp, (int32_t)  p.cond_handle);
}

/* this function was automagically generated */
static int32_t NaClSysCond_Timed_Wait_AbsDecoder(struct NaClAppThread *natp) {
  struct NaClSysCond_Timed_Wait_AbsArgs {
    uint32_t cond_handle;
    uint32_t mutex_handle;
    uint32_t ts;
  } p = *(struct NaClSysCond_Timed_Wait_AbsArgs *) natp->syscall_args;

  return NaClSysCond_Timed_Wait_Abs(natp, (int32_t)  p.cond_handle, (int32_t)  p.mutex_handle, (struct nacl_abi_timespec *) (uintptr_t) p.ts);
}

/* this function was automagically generated */
static int32_t NaClSysImc_SocketPairDecoder(struct NaClAppThread *natp) {
  struct NaClSysImc_SocketPairArgs {
    uint32_t d_out;
  } p = *(struct NaClSysImc_SocketPairArgs *) natp->syscall_args;

  return NaClSysImc_SocketPair(natp, (int32_t *) (uintptr_t) p.d_out);
}

/* this function was automagically generated */
static int32_t NaClSysSem_CreateDecoder(struct NaClAppThread *natp) {
  struct NaClSysSem_CreateArgs {
    uint32_t init_value;
  } p = *(struct NaClSysSem_CreateArgs *) natp->syscall_args;

  return NaClSysSem_Create(natp, (int32_t)  p.init_value);
}

/* this function was automagically generated */
static int32_t NaClSysSem_WaitDecoder(struct NaClAppThread *natp) {
  struct NaClSysSem_WaitArgs {
    uint32_t sem_handle;
  } p = *(struct NaClSysSem_WaitArgs *) natp->syscall_args;

  return NaClSysSem_Wait(natp, (int32_t)  p.sem_handle);
}

/* this function was automagically generated */
static int32_t NaClSysSem_PostDecoder(struct NaClAppThread *natp) {
  struct NaClSysSem_PostArgs {
    uint32_t sem_handle;
  } p = *(struct NaClSysSem_PostArgs *) natp->syscall_args;

  return NaClSysSem_Post(natp, (int32_t)  p.sem_handle);
}

/* this function was automagically generated */
static int32_t NaClSysSem_Get_ValueDecoder(struct NaClAppThread *natp) {
  struct NaClSysSem_Get_ValueArgs {
    uint32_t sem_handle;
  } p = *(struct NaClSysSem_Get_ValueArgs *) natp->syscall_args;

  return NaClSysSem_Get_Value(natp, (int32_t)  p.sem_handle);
}

/* this function was automagically generated */
static int32_t NaClSysSched_YieldDecoder(struct NaClAppThread *natp) {
  return NaClSysSched_Yield(natp);
}

/* this function was automagically generated */
static int32_t NaClSysSysconfDecoder(struct NaClAppThread *natp) {
  struct NaClSysSysconfArgs {
    uint32_t name;
    uint32_t result;
  } p = *(struct NaClSysSysconfArgs *) natp->syscall_args;

  return NaClSysSysconf(natp, (int32_t)  p.name, (int32_t *) (uintptr_t) p.result);
}

/* this function was automagically generated */
static int32_t NaClSysDyncode_CreateDecoder(struct NaClAppThread *natp) {
  struct NaClSysDyncode_CreateArgs {
    uint32_t dest;
    uint32_t src;
    uint32_t size;
  } p = *(struct NaClSysDyncode_CreateArgs *) natp->syscall_args;

  return NaClSysDyncode_Create(natp, (uint32_t)  p.dest, (uint32_t)  p.src, (uint32_t)  p.size);
}

/* this function was automagically generated */
static int32_t NaClSysDyncode_ModifyDecoder(struct NaClAppThread *natp) {
  struct NaClSysDyncode_ModifyArgs {
    uint32_t dest;
    uint32_t src;
    uint32_t size;
  } p = *(struct NaClSysDyncode_ModifyArgs *) natp->syscall_args;

  return NaClSysDyncode_Modify(natp, (uint32_t)  p.dest, (uint32_t)  p.src, (uint32_t)  p.size);
}

/* this function was automagically generated */
static int32_t NaClSysDyncode_DeleteDecoder(struct NaClAppThread *natp) {
  struct NaClSysDyncode_DeleteArgs {
    uint32_t dest;
    uint32_t size;
  } p = *(struct NaClSysDyncode_DeleteArgs *) natp->syscall_args;

  return NaClSysDyncode_Delete(natp, (uint32_t)  p.dest, (uint32_t)  p.size);
}

/* this function was automagically generated */
static int32_t NaClSysSecond_Tls_SetDecoder(struct NaClAppThread *natp) {
  struct NaClSysSecond_Tls_SetArgs {
    uint32_t new_value;
  } p = *(struct NaClSysSecond_Tls_SetArgs *) natp->syscall_args;

  return NaClSysSecond_Tls_Set(natp, (uint32_t)  p.new_value);
}

/* this function was automagically generated */
static int32_t NaClSysSecond_Tls_GetDecoder(struct NaClAppThread *natp) {
  return NaClSysSecond_Tls_Get(natp);
}

/* this function was automagically generated */
static int32_t NaClSysTest_InfoLeakDecoder(struct NaClAppThread *natp) {
  return NaClSysTest_InfoLeak(natp);
}

/*
 * d'b: "One Ring" syscall hook. tonneling syscall. all system calls go trough it
 * will be the only syscall in the future.
 * accepts 2 parameters:
 *   1st (input) - pointer to uint64_t (from user side it is array of uint64_t's)
 *   2nd (output)- pointer to uint64_t (from user side it is array of uint64_t's)
 * returns int32_t
 * note: [0] element from input is always syscall id; [1].. are syscalls parameters
 */
static int32_t OneRingDecoder(struct NaClApp *nap)
{
  struct OneRingDecoderArgs
  {
    uint32_t args;
  } p = *(struct OneRingDecoderArgs*) nap->syscall_args;

  return TrapHandler(nap, p.args);
}

/* d'b: put to the log restricted syscall detection */
static int32_t NaClSysRestricted()
{
  if(!NaClSilentRestrictedSyscallsEnabled())
    NaClLog(LOG_FATAL, "Restricted syscall. Abort.\n");
  return -NACL_ABI_ENOSYS;
}

/* auto generated */
void NaClSyscallTableInit() {
  int i;
  for (i = 0; i < NACL_MAX_SYSCALLS; ++i) {
    nacl_syscall[i].handler = &NotImplementedDecoder;
  }

  NaClAddSyscall(One_ring, &OneRingDecoder); /* d'b: 0. added onering call */
  NaClAddSyscall(NACL_sys_null, &NaClSysNullDecoder);
  NaClAddSyscall(NACL_sys_nameservice, &NaClSysNameServiceDecoder);
  NaClAddSyscall(NACL_sys_dup, &NaClSysDupDecoder);
  NaClAddSyscall(NACL_sys_dup2, &NaClSysDup2Decoder);
  NaClAddSyscall(NACL_sys_open, &NaClSysOpenDecoder);
  NaClAddSyscall(NACL_sys_close, &NaClSysCloseDecoder);
  NaClAddSyscall(NACL_sys_read, &NaClSysReadDecoder);
  NaClAddSyscall(NACL_sys_write, &NaClSysWriteDecoder);
  NaClAddSyscall(NACL_sys_lseek, &NaClSysLseekDecoder);
  NaClAddSyscall(NACL_sys_ioctl, &NaClSysIoctlDecoder);
  NaClAddSyscall(NACL_sys_fstat, &NaClSysFstatDecoder);
  NaClAddSyscall(NACL_sys_stat, &NaClSysStatDecoder);
  NaClAddSyscall(NACL_sys_getdents, &NaClSysGetdentsDecoder);
  NaClAddSyscall(NACL_sys_sysbrk, &NaClSysSysbrkDecoder);
  NaClAddSyscall(NACL_sys_mmap, &NaClSysMmapDecoder);
  NaClAddSyscall(NACL_sys_munmap, &NaClSysMunmapDecoder);
  NaClAddSyscall(NACL_sys_exit, &NaClSysExitDecoder);
  NaClAddSyscall(NACL_sys_getpid, &NaClSysGetpidDecoder);
  NaClAddSyscall(NACL_sys_thread_exit, &NaClSysThread_ExitDecoder);
  NaClAddSyscall(NACL_sys_gettimeofday, &NaClSysGetTimeOfDayDecoder);
  NaClAddSyscall(NACL_sys_clock, &NaClSysClockDecoder);
  NaClAddSyscall(NACL_sys_nanosleep, &NaClSysNanosleepDecoder);
  NaClAddSyscall(NACL_sys_imc_makeboundsock, &NaClSysImc_MakeBoundSockDecoder);
  NaClAddSyscall(NACL_sys_imc_accept, &NaClSysImc_AcceptDecoder);
  NaClAddSyscall(NACL_sys_imc_connect, &NaClSysImc_ConnectDecoder);
  NaClAddSyscall(NACL_sys_imc_sendmsg, &NaClSysImc_SendmsgDecoder);
  NaClAddSyscall(NACL_sys_imc_recvmsg, &NaClSysImc_RecvmsgDecoder);
  NaClAddSyscall(NACL_sys_imc_mem_obj_create, &NaClSysImc_Mem_Obj_CreateDecoder);
  NaClAddSyscall(NACL_sys_tls_init, &NaClSysTls_InitDecoder);
  NaClAddSyscall(NACL_sys_thread_create, &NaClSysThread_CreateDecoder);
  NaClAddSyscall(NACL_sys_tls_get, &NaClSysTls_GetDecoder);
  NaClAddSyscall(NACL_sys_thread_nice, &NaClSysThread_NiceDecoder);
  NaClAddSyscall(NACL_sys_mutex_create, &NaClSysMutex_CreateDecoder);
  NaClAddSyscall(NACL_sys_mutex_lock, &NaClSysMutex_LockDecoder);
  NaClAddSyscall(NACL_sys_mutex_unlock, &NaClSysMutex_UnlockDecoder);
  NaClAddSyscall(NACL_sys_mutex_trylock, &NaClSysMutex_TrylockDecoder);
  NaClAddSyscall(NACL_sys_cond_create, &NaClSysCond_CreateDecoder);
  NaClAddSyscall(NACL_sys_cond_wait, &NaClSysCond_WaitDecoder);
  NaClAddSyscall(NACL_sys_cond_signal, &NaClSysCond_SignalDecoder);
  NaClAddSyscall(NACL_sys_cond_broadcast, &NaClSysCond_BroadcastDecoder);
  NaClAddSyscall(NACL_sys_cond_timed_wait_abs, &NaClSysCond_Timed_Wait_AbsDecoder);
  NaClAddSyscall(NACL_sys_imc_socketpair, &NaClSysImc_SocketPairDecoder);
  NaClAddSyscall(NACL_sys_sem_create, &NaClSysSem_CreateDecoder);
  NaClAddSyscall(NACL_sys_sem_wait, &NaClSysSem_WaitDecoder);
  NaClAddSyscall(NACL_sys_sem_post, &NaClSysSem_PostDecoder);
  NaClAddSyscall(NACL_sys_sem_get_value, &NaClSysSem_Get_ValueDecoder);
  NaClAddSyscall(NACL_sys_sched_yield, &NaClSysSched_YieldDecoder);
  NaClAddSyscall(NACL_sys_sysconf, &NaClSysSysconfDecoder);
  NaClAddSyscall(NACL_sys_dyncode_create, &NaClSysDyncode_CreateDecoder);
  NaClAddSyscall(NACL_sys_dyncode_modify, &NaClSysDyncode_ModifyDecoder);
  NaClAddSyscall(NACL_sys_dyncode_delete, &NaClSysDyncode_DeleteDecoder);
  NaClAddSyscall(NACL_sys_second_tls_set, &NaClSysSecond_Tls_SetDecoder);
  NaClAddSyscall(NACL_sys_second_tls_get, &NaClSysSecond_Tls_GetDecoder);
  NaClAddSyscall(NACL_sys_test_infoleak, &NaClSysTest_InfoLeakDecoder);
}

/*
  d'b:
  in this list of system calls 5 ones left untouched:
  (other syscalls set to "NaClSysRestricted")

  syscalls for an empty nexe: "main(){}"
  +  20 -- NACL_sys_sysbrk (NaClSysSysbrkDecoder) -- still need to serve user malloc()
  ++ 30 -- NACL_sys_exit (NaClSysExitDecoder) -- no need. i gonna exit through long/short jump
  +  70 -- NACL_sys_mutex_create (NaClSysMutex_CreateDecoder) -- dummy
  +  82 -- NACL_sys_tls_init (NaClSysTls_InitDecoder) -- dummy
  +  84 -- NACL_sys_tls_get (NaClSysTls_GetDecoder) -- dummy

  special case, tonneling call:
  0 -- artificially added "One Ring" single syscall

  note: to restore syscalls to the original version just delete everything
  until "d'b end" and uncomment commented code
*/
void NaClSyscallTableInitDisable() {
  int i;
  for (i = 0; i < NACL_MAX_SYSCALLS; ++i) {
     nacl_syscall[i].handler = &NotImplementedDecoder;
  }

  NaClAddSyscall(One_ring, &OneRingDecoder); /* 0. added onering call */
  NaClAddSyscall(NACL_sys_null, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_nameservice, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_dup, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_dup2, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_open, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_close, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_read, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_write, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_lseek, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_ioctl, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_fstat, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_stat, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_getdents, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sysbrk, &NaClSysSysbrkDecoder); /* 20 */
  NaClAddSyscall(NACL_sys_mmap, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_munmap, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_exit, &NaClSysExitDecoder); /* 30 */
  NaClAddSyscall(NACL_sys_getpid, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_thread_exit, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_gettimeofday, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_clock, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_nanosleep, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_makeboundsock, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_accept, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_connect, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_sendmsg, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_recvmsg, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_mem_obj_create, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_tls_init, &NaClSysTls_InitDecoder); /* 82 */
  NaClAddSyscall(NACL_sys_thread_create, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_tls_get, &NaClSysTls_GetDecoder); /* 84 */
  NaClAddSyscall(NACL_sys_thread_nice, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_mutex_create, &NaClSysMutex_CreateDecoder); /* 70 */
  NaClAddSyscall(NACL_sys_mutex_lock, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_mutex_unlock, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_mutex_trylock, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_cond_create, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_cond_wait, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_cond_signal, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_cond_broadcast, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_cond_timed_wait_abs, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_imc_socketpair, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sem_create, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sem_wait, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sem_post, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sem_get_value, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sched_yield, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_sysconf, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_dyncode_create, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_dyncode_modify, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_dyncode_delete, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_second_tls_set, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_second_tls_get, &NaClSysRestricted);
  NaClAddSyscall(NACL_sys_test_infoleak, &NaClSysRestricted);
}
/* d'b end */
