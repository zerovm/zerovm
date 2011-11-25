/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl service run-time, non-platform specific system call helper routines.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>

#include "native_client/src/trusted/service_runtime/nacl_syscall_common.h"

#include "native_client/src/include/nacl_assert.h"
#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/nacl_platform.h"
#include "native_client/src/include/portability_string.h"

#include "native_client/src/shared/platform/nacl_exit.h"
#include "native_client/src/shared/platform/nacl_host_desc.h"
#include "native_client/src/shared/platform/nacl_host_dir.h"
#include "native_client/src/shared/platform/nacl_sync_checked.h"

#include "native_client/src/trusted/desc/nacl_desc_base.h"
#include "native_client/src/trusted/desc/nacl_desc_cond.h"
#include "native_client/src/trusted/desc/nacl_desc_dir.h"
#include "native_client/src/trusted/desc/nacl_desc_imc.h"
#include "native_client/src/trusted/desc/nacl_desc_imc_shm.h"
#include "native_client/src/trusted/desc/nacl_desc_invalid.h"
#include "native_client/src/trusted/desc/nacl_desc_io.h"
#include "native_client/src/trusted/desc/nacl_desc_mutex.h"
#include "native_client/src/trusted/desc/nacl_desc_semaphore.h"
#include "native_client/src/trusted/desc/nrd_xfer.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"
#include "native_client/src/trusted/service_runtime/nacl_app_thread.h"
#include "native_client/src/trusted/service_runtime/nacl_thread_nice.h"
#include "native_client/src/trusted/service_runtime/nacl_globals.h"
#include "native_client/src/trusted/service_runtime/nacl_tls.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"
#include "native_client/src/trusted/service_runtime/sel_memory.h"

#include "native_client/src/trusted/service_runtime/include/sys/errno.h"
#include "native_client/src/trusted/service_runtime/include/sys/fcntl.h"
#include "native_client/src/trusted/service_runtime/include/sys/mman.h"
#include "native_client/src/trusted/service_runtime/include/sys/stat.h"


/*
 * OSX defines SIZE_T_MAX in i386/limits.h; Linux has SIZE_MAX;
 * Windows has none.
 *
 * TODO(bsy): remove when we put SIZE_T_MAX in a common header file.
 */
#if !defined(SIZE_T_MAX)
# define SIZE_T_MAX   (~(size_t) 0)
#endif

static const size_t kMaxUsableFileSize = (SIZE_T_MAX >> 1);

static INLINE size_t  size_min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

static int const kKnownInvalidDescNumber = -1;

void NaClSysCommonThreadSyscallEnter(struct NaClAppThread *natp) {
  UNREFERENCED_PARAMETER(natp);
}

void NaClSysCommonThreadSyscallLeave(struct NaClAppThread *natp) {
  NaClXMutexLock(&natp->mu);
  switch (natp->state) {
    case NACL_APP_THREAD_ALIVE:
      break;
    case NACL_APP_THREAD_SUICIDE_PENDING:
      NaClXMutexUnlock(&natp->mu);
      NaClAppThreadTeardown(natp);
      /* NOTREACHED */
      break;
    case NACL_APP_THREAD_DEAD:
      NaClLog(LOG_FATAL, "Dead thread at NaClSysCommonThreadSyscallLeave\n");
      /* NOTREACHED */
      break;
  }
  NaClXMutexUnlock(&natp->mu);
}

int32_t NaClSetBreak(struct NaClAppThread *natp,
                     uintptr_t            new_break) {
  struct NaClApp        *nap;
  uintptr_t             break_addr;
  int32_t               rv = -NACL_ABI_EINVAL;
  struct NaClVmmapIter  iter;
  struct NaClVmmapEntry *ent;
  struct NaClVmmapEntry *next_ent;
  uintptr_t             sys_break;
  uintptr_t             sys_new_break;
  uintptr_t             usr_last_data_page;
  uintptr_t             usr_new_last_data_page;
  uintptr_t             last_internal_data_addr;
  uintptr_t             last_internal_page;
  uintptr_t             start_new_region;
  uintptr_t             region_size;

  nap = natp->nap;
  break_addr = nap->break_addr;

  NaClLog(3, "Entered NaClSetBreak(new_break 0x%08"NACL_PRIxPTR")\n",
          new_break);

  NaClSysCommonThreadSyscallEnter(natp);

  sys_new_break = NaClUserToSysAddr(natp->nap, new_break);
  NaClLog(3, "sys_new_break 0x%08"NACL_PRIxPTR"\n", sys_new_break);

  if (kNaClBadAddress == sys_new_break) {
    goto cleanup_no_lock;
  }
  if (NACL_SYNC_OK != NaClMutexLock(&nap->mu)) {
    NaClLog(LOG_ERROR, "Could not get app lock for 0x%08"NACL_PRIxPTR"\n",
            (uintptr_t) nap);
    goto cleanup_no_lock;
  }
  if (new_break < nap->data_end) {
    NaClLog(4, "new_break before data_end (0x%"NACL_PRIxPTR")\n",
            nap->data_end);
    goto cleanup;
  }
  if (new_break <= nap->break_addr) {
    /* freeing memory */
    NaClLog(4, "new_break before break (0x%"NACL_PRIxPTR"); freeing\n",
            nap->break_addr);
    nap->break_addr = new_break;
    break_addr = new_break;
  } else {

    /*
     * See if page containing new_break is in mem_map; if so, we are
     * essentially done -- just update break_addr.  Otherwise, we
     * extend the VM map entry from the page containing the current
     * break to the page containing new_break.
     */

    sys_break = NaClUserToSys(nap, nap->break_addr);

    usr_last_data_page = (nap->break_addr - 1) >> NACL_PAGESHIFT;

    usr_new_last_data_page = (new_break - 1) >> NACL_PAGESHIFT;

    last_internal_data_addr = NaClRoundAllocPage(new_break) - 1;
    last_internal_page = last_internal_data_addr >> NACL_PAGESHIFT;

    NaClLog(4, ("current break sys addr 0x%08"NACL_PRIxPTR", "
                "usr last data page 0x%"NACL_PRIxPTR"\n"),
            sys_break, usr_last_data_page);
    NaClLog(4, "new break usr last data page 0x%"NACL_PRIxPTR"\n",
            usr_new_last_data_page);
    NaClLog(4, "last internal data addr 0x%08"NACL_PRIxPTR"\n",
            last_internal_data_addr);

    if (NULL == NaClVmmapFindPageIter(&nap->mem_map,
                                      usr_last_data_page,
                                      &iter)
        || NaClVmmapIterAtEnd(&iter)) {
      NaClLog(LOG_FATAL, ("current break (0x%08"NACL_PRIxPTR", "
                          "sys 0x%08"NACL_PRIxPTR") "
                          "not in address map\n"),
              nap->break_addr, sys_break);
    }
    ent = NaClVmmapIterStar(&iter);
    NaClLog(4, ("segment containing current break"
                ": page_num 0x%08"NACL_PRIxPTR", npages 0x%"NACL_PRIxS"\n"),
            ent->page_num, ent->npages);
    if (usr_new_last_data_page < ent->page_num + ent->npages) {
      NaClLog(4, "new break within break segment, just bumping addr\n");
      nap->break_addr = new_break;
      break_addr = new_break;
    } else {
      NaClVmmapIterIncr(&iter);
      if (!NaClVmmapIterAtEnd(&iter)
          && ((next_ent = NaClVmmapIterStar(&iter))->page_num
              <= last_internal_page)) {
        /* ran into next segment! */
        NaClLog(4,
                ("new break request of usr address "
                 "0x%08"NACL_PRIxPTR" / usr page 0x%"NACL_PRIxPTR
                 " runs into next region, page_num 0x%"NACL_PRIxPTR", "
                 "npages 0x%"NACL_PRIxS"\n"),
                new_break, usr_new_last_data_page,
                next_ent->page_num, next_ent->npages);
        goto cleanup;
      }
      NaClLog(4,
              "extending segment: page_num 0x%08"NACL_PRIxPTR", "
              "npages 0x%"NACL_PRIxS"\n",
              ent->page_num, ent->npages);
      /* go ahead and extend ent to cover, and make pages accessible */
      start_new_region = (ent->page_num + ent->npages) << NACL_PAGESHIFT;
      ent->npages = (last_internal_page - ent->page_num + 1);
      region_size = (((last_internal_page + 1) << NACL_PAGESHIFT)
                     - start_new_region);
      if (0 != NaCl_mprotect((void *) NaClUserToSys(nap, start_new_region),
                             region_size,
                             PROT_READ | PROT_WRITE)) {
        NaClLog(LOG_FATAL,
                ("Could not mprotect(0x%08"NACL_PRIxPTR", "
                 "0x%08"NACL_PRIxPTR", "
                 "PROT_READ|PROT_WRITE)\n"),
                start_new_region,
                region_size);
      }
      NaClLog(4, "segment now: page_num 0x%08"NACL_PRIxPTR", "
              "npages 0x%"NACL_PRIxS"\n",
              ent->page_num, ent->npages);
      nap->break_addr = new_break;
      break_addr = new_break;
    }
  }



cleanup:
  NaClXMutexUnlock(&nap->mu);
cleanup_no_lock:
  NaClSysCommonThreadSyscallLeave(natp);

  /*
   * This cast is safe because the incoming value (new_break) cannot
   * exceed the user address space--even though its type (uintptr_t)
   * theoretically allows larger values.
   */
  rv = (int32_t) break_addr;

  NaClLog(3, "NaClSetBreak: returning 0x%08"NACL_PRIx32"\n", rv);
  return rv;
}

int NaClAclBypassChecks = 0;

void NaClInsecurelyBypassAllAclChecks(void) {
  NaClLog(LOG_WARNING, "BYPASSING ALL ACL CHECKS\n");
  NaClAclBypassChecks = 1;
}

int NaClHighResolutionTimerEnabled() {
  return NaClAclBypassChecks;
}

/*
 * NaClOpenAclCheck: Is the NaCl app authorized to open this file?  The
 * return value is syscall return convention, so 0 is success and
 * small negative numbers are negated errno values.
 */
int32_t NaClOpenAclCheck(struct NaClApp *nap,
                         char const     *path,
                         int            flags,
                         int            mode) {
  /*
   * TODO(bsy): provide some minimal authorization check, based on
   * whether a debug flag is set; eventually provide a data-driven
   * authorization configuration mechanism, perhaps persisted via
   * gears.  need GUI for user configuration, as well as designing an
   * appropriate language (with sufficient expressiveness), however.
   */
  NaClLog(1, "NaClOpenAclCheck(0x%08"NACL_PRIxPTR", %s, 0%o, 0%o)\n",
          (uintptr_t) nap, path, flags, mode);
  if (3 < NaClLogGetVerbosity()) {
    NaClLog(0, "O_ACCMODE: 0%o\n", flags & NACL_ABI_O_ACCMODE);
    NaClLog(0, "O_RDONLY = %d\n", NACL_ABI_O_RDONLY);
    NaClLog(0, "O_WRONLY = %d\n", NACL_ABI_O_WRONLY);
    NaClLog(0, "O_RDWR   = %d\n", NACL_ABI_O_RDWR);
#define FLOG(VAR, BIT) do {\
      NaClLog(1, "%s: %s\n", #BIT, (VAR & BIT) ? "yes" : "no");\
    } while (0)
    FLOG(flags, NACL_ABI_O_CREAT);
    FLOG(flags, NACL_ABI_O_TRUNC);
    FLOG(flags, NACL_ABI_O_APPEND);
#undef FLOG
  }
  if (NaClAclBypassChecks) {
    return 0;
  }
  return -NACL_ABI_EACCES;
}

/*
 * NaClStatAclCheck: Is the NaCl app authorized to stat this pathname?  The
 * return value is syscall return convention, so 0 is success and
 * small negative numbers are negated errno values.
 *
 * This is primarily for debug use.  File access should be through
 * SRPC-based file servers.
 */
int32_t NaClStatAclCheck(struct NaClApp *nap,
                         char const     *path) {
  NaClLog(2,
          "NaClStatAclCheck(0x%08"NACL_PRIxPTR", %s)\n", (uintptr_t) nap, path);
  if (NaClAclBypassChecks) {
    return 0;
  }
  return -NACL_ABI_EACCES;
}

int32_t NaClIoctlAclCheck(struct NaClApp  *nap,
                          struct NaClDesc *ndp,
                          int             request,
                          void            *arg) {
  NaClLog(2,
          ("NaClIoctlAclCheck(0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR","
           " %d, 0x%08"NACL_PRIxPTR"\n"),
          (uintptr_t) nap, (uintptr_t) ndp, request, (uintptr_t) arg);
  if (NaClAclBypassChecks) {
    return 0;
  }
  return -NACL_ABI_EINVAL;
}

int32_t NaClCommonSysExit(struct NaClAppThread  *natp,
                          int                   status) {
  struct NaClApp            *nap;

  NaClLog(1, "Exit syscall handler: %d\n", status);
  NaClSysCommonThreadSyscallEnter(natp);

  nap = natp->nap;

  (void) NaClReportExitStatus(nap, status);

  NaClAppThreadTeardown(natp);
  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

int32_t NaClCommonSysThreadExit(struct NaClAppThread  *natp,
                                int32_t               *stack_flag) {
  uintptr_t sys_stack_flag;
  struct NaClApp  *nap;

  NaClLog(4, "NaclCommonSysThreadExit(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) natp,
          (uintptr_t) stack_flag);
  NaClSysCommonThreadSyscallEnter(natp);
  /*
   * NB: NaClThreads are never joinable, but the abstraction for NaClApps
   * are.
   */
  nap = natp->nap;

  if (NULL != stack_flag) {
    NaClLog(4,
            "NaClCommonSysThreadExit: stack_flag is %"NACL_PRIxPTR"\n",
            (uintptr_t) stack_flag);
    sys_stack_flag = NaClUserToSysAddrRange(natp->nap,
                                            (uintptr_t) stack_flag,
                                            sizeof(int32_t));
    NaClLog(4,
            "NaClCommonSysThreadExit: sys_stack_flag is %"NACL_PRIxPTR"\n",
            sys_stack_flag);
    if (kNaClBadAddress != sys_stack_flag) {
      /*
       * We don't return failure if the address is illegal because
       * this function is not supposed to return.
       */
      NaClLog(4,
              "NaClCommonSysThreadExit: clearing stack flag\n");
      *(int32_t *) sys_stack_flag = 0;
    }
  }

  NaClAppThreadTeardown(natp);
  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

int32_t NaClCommonSysNameService(struct NaClAppThread *natp,
                                 int32_t              *desc_addr) {
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sysaddr;
  int32_t   desc;

  NaClLog(3,
          ("NaClCommonSysNameService(0x%08"NACL_PRIxPTR","
           " 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp,
          (uintptr_t) desc_addr);
  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) desc_addr,
                                   sizeof(uint32_t));
  if (kNaClBadAddress == sysaddr) {
    NaClLog(LOG_ERROR,
            "Invalid address argument to NaClCommonSysNameService\n");
    retval = -NACL_ABI_EFAULT;
    goto done;
  }

  desc = *(int32_t volatile *) sysaddr;
  if (-1 == desc) {
    /* read */
    desc = NaClSetAvail(natp->nap,
                        NaClDescRef(natp->nap->name_service_conn_cap));
    *(int32_t volatile *) sysaddr = desc;
    retval = 0;
  } else {
    struct NaClDesc *desc_obj_ptr = NaClGetDesc(natp->nap, desc);

    if (NULL == desc_obj_ptr) {
      retval = -NACL_ABI_EBADF;
      goto done;
    }
    if (NACL_DESC_CONN_CAP != NACL_VTBL(NaClDesc, desc_obj_ptr)->typeTag &&
        NACL_DESC_CONN_CAP_FD != NACL_VTBL(NaClDesc, desc_obj_ptr)->typeTag) {
      retval = -NACL_ABI_EINVAL;
      goto done;
    }
    /* write */
    NaClXMutexLock(&natp->nap->mu);
    NaClDescUnref(natp->nap->name_service_conn_cap);
    natp->nap->name_service_conn_cap = desc_obj_ptr;
    NaClXMutexUnlock(&natp->nap->mu);
    retval = 0;
  }

 done:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysDup(struct NaClAppThread *natp,
                         int                  oldfd) {
  int             retval;
  struct NaClDesc *old_nd;

  NaClLog(3, "NaClCommonSysDup(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, oldfd);
  NaClSysCommonThreadSyscallEnter(natp);
  old_nd = NaClGetDesc(natp->nap, oldfd);
  if (NULL == old_nd) {
    retval = -NACL_ABI_EBADF;
    goto done;
  }
  retval = NaClSetAvail(natp->nap, old_nd);
done:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysDup2(struct NaClAppThread  *natp,
                          int                   oldfd,
                          int                   newfd) {
  int             retval;
  struct NaClDesc *old_nd;

  NaClLog(3, "NaClCommonSysDup(0x%08"NACL_PRIxPTR", %d, %d)\n",
          (uintptr_t) natp, oldfd, newfd);
  NaClSysCommonThreadSyscallEnter(natp);
  if (newfd < 0) {
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  /*
   * TODO(bsy): is this a reasonable largest sane value?  The
   * descriptor array shouldn't get too large.
   */
  if (newfd >= NACL_MAX_FD) {
    retval = -NACL_ABI_EINVAL;
    goto done;
  }
  old_nd = NaClGetDesc(natp->nap, oldfd);
  if (NULL == old_nd) {
    retval = -NACL_ABI_EBADF;
    goto done;
  }
  NaClSetDesc(natp->nap, newfd, old_nd);
  retval = newfd;
done:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysOpen(struct NaClAppThread  *natp,
                          char                  *pathname,
                          int                   flags,
                          int                   mode) {
  uint32_t             retval = -NACL_ABI_EINVAL;
  uintptr_t            sysaddr;
  char                 path[NACL_CONFIG_PATH_MAX];
  size_t               len;
  nacl_host_stat_t     stbuf;
  int                  allowed_flags;

  NaClLog(3, "NaClCommonSysOpen(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIxPTR", 0x%x, 0x%x)\n",
          (uintptr_t) natp, (uintptr_t) pathname, flags, mode);

  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddr(natp->nap, (uintptr_t) pathname);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(LOG_ERROR, "Invalid address for pathname\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  allowed_flags = (NACL_ABI_O_ACCMODE | NACL_ABI_O_CREAT
                   | NACL_ABI_O_TRUNC | NACL_ABI_O_APPEND);
  if (0 != (flags & ~allowed_flags)) {
    NaClLog(LOG_WARNING, "Invalid open flags 0%o, ignoring extraneous bits\n",
            flags);
    flags &= allowed_flags;
  }
  if (0 != (mode & ~0600)) {
    NaClLog(1, "IGNORING Invalid access mode bits 0%o\n", mode);
    mode &= 0600;
  }
  NaClLog(4, " attempting to copy path via sysaddr 0x%08"NACL_PRIxPTR
          "\n", sysaddr);
  NaClLog(4, " first 4 bytes: %.4s\n", (char *) sysaddr);
  /*
   * strncpy may (try to) get bytes that is outside the app's address
   * space and generate a fault.
   */
  strncpy(path, (char *) sysaddr, sizeof path);
  /*
   * survived the copy, but did there happen to be data beyond the end?
   */
  path[sizeof path - 1] = '\0';  /* always null terminate */
  NaClLog(1, "NaClSysOpen: Path: %s\n", path);
  len = strlen(path);
  /*
   * make sure sysaddr is a string, and the whole string is in app
   * address space...
   *
   * address space is convex, so it is impossible for beginning and
   * end to be both in the address space and yet have an intermediate
   * byte not be in the address space.
   */
  if (kNaClBadAddress == NaClUserToSysAddr(natp->nap,
                                           len + (uintptr_t) pathname)) {
    NaClLog(LOG_ERROR, "String ends outside addrspace\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  retval = NaClOpenAclCheck(natp->nap, path, flags, mode);
  if (0 != retval) {
    NaClLog(3, "Open ACL check rejected \"%s\".\n", path);
    goto cleanup;
  }

  /*
   * Perform a stat to determine whether the file is a directory.
   *
   * NB: it is okay for the stat to fail, since the request may be to
   * create a new file.
   *
   * There is a race conditions here: between the stat and the
   * open-as-a-file and open-as-a-dir, the type of the object that the
   * path refers to can change.
   */
  retval = NaClHostDescStat(path, &stbuf);

  /* Windows does not have S_ISDIR(m) macro */
  if (0 == retval && S_IFDIR == (S_IFDIR & stbuf.st_mode)) {
    struct NaClHostDir  *hd;

    hd = malloc(sizeof *hd);
    if (NULL == hd) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup;
    }
    retval = NaClHostDirOpen(hd, path);
    NaClLog(1, "NaClHostDirOpen(0x%08"NACL_PRIxPTR", %s) returned %d\n",
            (uintptr_t) hd, path, retval);
    if (0 == retval) {
      retval = NaClSetAvail(natp->nap,
                            ((struct NaClDesc *) NaClDescDirDescMake(hd)));
      NaClLog(1, "Entered directory into open file table at %d\n",
              retval);
    }
  } else {
    struct NaClHostDesc  *hd;

    hd = malloc(sizeof *hd);
    if (NULL == hd) {
      retval = -NACL_ABI_ENOMEM;
      goto cleanup;
    }
    retval = NaClHostDescOpen(hd, path, flags, mode);
    NaClLog(1,
            "NaClHostDescOpen(0x%08"NACL_PRIxPTR", %s, 0%o, 0%o) returned %d\n",
            (uintptr_t) hd, path, flags, mode, retval);
    if (0 == retval) {
      retval = NaClSetAvail(natp->nap,
                            ((struct NaClDesc *) NaClDescIoDescMake(hd)));
      NaClLog(1, "Entered into open file table at %d\n", retval);
    }
  }
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysClose(struct NaClAppThread *natp,
                           int                  d) {
  int             retval = -NACL_ABI_EBADF;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClCommonSysClose(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  NaClSysCommonThreadSyscallEnter(natp);

  NaClXMutexLock(&natp->nap->desc_mu);
  ndp = NaClGetDescMu(natp->nap, d);
  if (NULL != ndp) {
    NaClSetDescMu(natp->nap, d, NULL);  /* Unref the desc_tbl */
  }
  NaClXMutexUnlock(&natp->nap->desc_mu);
  NaClLog(5, "Invoking Close virtual function of object 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) ndp);
  if (NULL != ndp) {
    NaClDescUnref(ndp);
    retval = 0;
  }

  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysGetdents(struct NaClAppThread *natp,
                              int                  d,
                              void                 *dirp,
                              size_t               count) {
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         getdents_ret;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;

  NaClLog(3,
          ("Entered NaClCommonSysGetdents(0x%08"NACL_PRIxPTR", "
           "%d, 0x%08"NACL_PRIxPTR", "
           "%"NACL_PRIdS"[0x%"NACL_PRIxS"])\n"),
          (uintptr_t) natp, d, (uintptr_t) dirp, count, count);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) dirp, count);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, " illegal address for directory data\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unref;
  }

  /*
   * Clamp count to INT32_MAX to avoid the possibility of Getdents returning
   * a value that is outside the range of an int32.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }
  getdents_ret = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Getdents)(ndp,
                            (void *) sysaddr,
                            count);
  if ((getdents_ret < INT32_MIN && !NaClSSizeIsNegErrno(&getdents_ret))
      || INT32_MAX < getdents_ret) {
    /* This should never happen, because we already clamped the input count */
    NaClLog(LOG_FATAL, "Overflow in Getdents: return value is %"NACL_PRIxS,
            getdents_ret);
  } else {
    retval = (int32_t) getdents_ret;
  }
  if (retval > 0) {
    NaClLog(4, "getdents returned %d bytes\n", retval);
    NaClLog(8, "getdents result: %.*s\n", retval, (char *) sysaddr);
  } else {
    NaClLog(4, "getdents returned %d\n", retval);
  }

cleanup_unref:
  NaClDescUnref(ndp);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysRead(struct NaClAppThread  *natp,
                          int                   d,
                          void                  *buf,
                          size_t                count) {
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         read_result = -NACL_ABI_EINVAL;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;


  NaClLog(3,
          ("Entered NaClCommonSysRead(0x%08"NACL_PRIxPTR", "
           "%d, 0x%08"NACL_PRIxPTR", "
           "%"NACL_PRIdS"[0x%"NACL_PRIxS"])\n"),
          (uintptr_t) natp, d, (uintptr_t) buf, count, count);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) buf, count);
  if (kNaClBadAddress == sysaddr) {
    NaClDescUnref(ndp);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  /*
   * The maximum length for read and write is INT32_MAX--anything larger and
   * the return value would overflow. Passing larger values isn't an error--
   * we'll just clamp the request size if it's too large.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }

  read_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                 Read)(ndp, (void *) sysaddr, count);
  if (read_result > 0) {
    NaClLog(4, "read returned %"NACL_PRIdS" bytes\n", read_result);
    NaClLog(8, "read result: %.*s\n",
           (int) read_result,
           (char *) sysaddr);
  } else {
    NaClLog(4, "read returned %"NACL_PRIdS"\n", read_result);
  }
  NaClDescUnref(ndp);

  /* This cast is safe because we clamped count above.*/
  retval = (int32_t) read_result;
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysWrite(struct NaClAppThread *natp,
                           int                  d,
                           void                 *buf,
                           size_t               count) {
  int32_t         retval = -NACL_ABI_EINVAL;
  ssize_t         write_result = -NACL_ABI_EINVAL;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;

  NaClLog(3,
          "Entered NaClCommonSysWrite(0x%08"NACL_PRIxPTR", "
          "%d, 0x%08"NACL_PRIxPTR", "
          "%"NACL_PRIdS"[0x%"NACL_PRIxS"])\n",
          (uintptr_t) natp, d, (uintptr_t) buf, count, count);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) buf, count);
  if (kNaClBadAddress == sysaddr) {
    NaClDescUnref(ndp);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClLog(4, "In NaClSysWrite(%d, %.*s, %"NACL_PRIdS")\n",
          d, (int) count, (char *) sysaddr, count);

  /*
   * The maximum length for read and write is INT32_MAX--anything larger and
   * the return value would overflow. Passing larger values isn't an error--
   * we'll just clamp the request size if it's too large.
   */
  if (count > INT32_MAX) {
    count = INT32_MAX;
  }

  write_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Write)(ndp, (void *) sysaddr, count);

  NaClDescUnref(ndp);

  /* This cast is safe because we clamped count above.*/
  retval = (int32_t) write_result;

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

/*
 * This is not lseek64, so the return value on success can be
 * E_OVERFLOW if it does not fit in a 32-bit off_t.
 */
int32_t NaClCommonSysLseek(struct NaClAppThread *natp,
                           int                  d,
                           nacl_abi_off_t       *offp,
                           int                  whence) {
  uintptr_t       sysaddr;
  nacl_abi_off_t  offset;
  nacl_off64_t    retval64;
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3,
          ("Entered NaClCommonSysLseek(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, d, (uintptr_t) offp, whence);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) offp, sizeof offset);
  if (kNaClBadAddress == sysaddr) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup_unref;
  }
  offset = *(nacl_abi_off_t volatile *) sysaddr;
  NaClLog(4, "offset 0x%08"NACL_PRIxNACL_OFF"\n", offset);

  retval64 = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              Seek)(ndp, (nacl_off64_t) offset, whence);
  if (NaClOff64IsNegErrno(&retval64)) {
    retval = (int32_t) retval64;
  } else {
    *(nacl_abi_off_t volatile *) sysaddr = retval64;
    retval = 0;
  }
cleanup_unref:
  NaClDescUnref(ndp);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysIoctl(struct NaClAppThread *natp,
                           int                  d,
                           int                  request,
                           void                 *arg) {
  int             retval = -NACL_ABI_EINVAL;
  uintptr_t       sysaddr;
  struct NaClDesc *ndp;

  NaClLog(3,
          ("Entered NaClSysIoctl(0x%08"NACL_PRIxPTR
           ", %d, %d, 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, d, request,
          (uintptr_t) arg);

  NaClSysCommonThreadSyscallEnter(natp);
  /*
   * Note that NaClUserToSysAddrRange is not feasible right now, since
   * the size of the arg argument depends on the request.  We do not
   * have an enumeration of allowed ioctl requests yet.
   *
   * Furthermore, some requests take no arguments, so sysaddr might
   * end up being kNaClBadAddress and that is perfectly okay.
   */
  sysaddr = NaClUserToSysAddr(natp->nap, (uintptr_t) arg);
  /*
   ****************************************
   * NOTE: sysaddr may be kNaClBadAddress *
   ****************************************
   */

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    NaClLog(4, "bad desc\n");
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = NaClIoctlAclCheck(natp->nap, ndp, request, arg);
  if (0 != retval) {
    NaClLog(3, "Ioctl ACL check rejected descriptor %d\n", d);
    goto cleanup_unref;
  }

  retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
            Ioctl)(ndp, request, (void *) sysaddr);
cleanup_unref:
  NaClDescUnref(ndp);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}


int32_t NaClCommonSysFstat(struct NaClAppThread *natp,
                           int                  d,
                           struct nacl_abi_stat *nasp) {
  int32_t               retval = -NACL_ABI_EINVAL;
  uintptr_t             sysaddr;
  struct NaClDesc       *ndp;

  NaClLog(3,
          ("Entered NaClSysFstat(0x%08"NACL_PRIxPTR
           ", %d, 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp,
          d, (uintptr_t) nasp);

  NaClSysCommonThreadSyscallEnter(natp);

  NaClLog(4,
          " sizeof(struct nacl_abi_stat) = %"NACL_PRIdS" (0x%"NACL_PRIxS")\n",
          sizeof *nasp, sizeof *nasp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    NaClLog(4, "bad desc\n");
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) nasp, sizeof *nasp);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "bad addr\n");
    retval = -NACL_ABI_EFAULT;
  } else {
    retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              Fstat)(ndp, (struct nacl_abi_stat *) sysaddr);
  }

  NaClDescUnref(ndp);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysStat(struct NaClAppThread  *natp,
                          const char            *pathname,
                          struct nacl_abi_stat  *buf) {
  int32_t             retval = -NACL_ABI_EINVAL;
  uintptr_t           syspathaddr;
  uintptr_t           sysbufaddr;
  char                path[NACL_CONFIG_PATH_MAX];
  size_t              len;
  nacl_host_stat_t    stbuf;

  NaClLog(3,
          ("Entered NaClCommonSysStat(0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR","
           " 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) pathname, (uintptr_t) buf);

  NaClSysCommonThreadSyscallEnter(natp);

  syspathaddr = NaClUserToSysAddr(natp->nap, (uintptr_t) pathname);
  if (kNaClBadAddress == syspathaddr) {
    NaClLog(LOG_ERROR, "Invalid address for pathname\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  /*
   * strncpy may (try to) get bytes that is outside the app's address
   * space and generate a fault.
   */
  strncpy(path, (char *) syspathaddr, sizeof path);
  /*
   * survived the copy, but did there happen to be data beyond the end?
   */
  path[sizeof path - 1] = '\0';  /* always null terminate */
  NaClLog(2, "NaClCommonSysStat: Path: %s\n", path);
  len = strlen(path);
  /*
   * make sure sysaddr is a string, and the whole string is in app
   * address space...
   *
   * address space is convex, so it is impossible for beginning and
   * end to be both in the address space and yet have an intermediate
   * byte not be in the address space.
   */
  if (kNaClBadAddress == NaClUserToSysAddr(natp->nap,
                                           len + (uintptr_t) pathname)) {
    NaClLog(LOG_ERROR, "String ends outside addrspace\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  /*
   * Make sure result buffer is in the app's address space.
   */
  sysbufaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) buf, sizeof *buf);
  if (kNaClBadAddress == sysbufaddr) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  retval = NaClStatAclCheck(natp->nap, path);
  if (0 != retval) {
    goto cleanup;
  }

  /*
   * Perform a host stat.
   */
  retval = NaClHostDescStat(path, &stbuf);
  if (0 == retval) {
    retval = NaClAbiStatHostDescStatXlateCtor((struct nacl_abi_stat *)
                                              sysbufaddr,
                                              &stbuf);
  }
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

void NaClCommonUtilUpdateAddrMap(struct NaClApp       *nap,
                                 uintptr_t            sysaddr,
                                 size_t               nbytes,
                                 int                  sysprot,
                                 struct NaClDesc      *backing_desc,
                                 nacl_off64_t         backing_bytes,
                                 nacl_off64_t         offset_bytes,
                                 int                  delete_mem) {
  uintptr_t                   usraddr;
  struct NaClMemObj           *nmop;

  NaClLog(3,
          ("NaClCommonUtilUpdateAddrMap(0x%08"NACL_PRIxPTR", "
           "0x%08"NACL_PRIxPTR", "
           "0x%"NACL_PRIxS", 0x%x, 0x%08"NACL_PRIxPTR", 0x%"NACL_PRIx64", "
           "0x%"NACL_PRIx64", %d)\n"),
          (uintptr_t) nap, sysaddr, nbytes,
          sysprot, (uintptr_t) backing_desc, backing_bytes,
          offset_bytes,
          delete_mem);
  usraddr = NaClSysToUser(nap, sysaddr);
  nmop = NULL;
  /* delete_mem -> NULL == backing_desc */
  if (NULL != backing_desc) {
    if (delete_mem) {
      NaClLog(LOG_FATAL,
              ("invariant of delete_mem implies backing_desc NULL"
               " violated.\n"));
    }
    nmop = NaClMemObjMake(backing_desc, backing_bytes, offset_bytes);
  }

  NaClVmmapUpdate(&nap->mem_map,
                  usraddr >> NACL_PAGESHIFT,
                  nbytes >> NACL_PAGESHIFT,
                  sysprot,
                  nmop,
                  delete_mem);
}


int NaClSysCommonAddrRangeContainsExecutablePages_mu(struct NaClApp *nap,
                                                     uintptr_t      usraddr,
                                                     size_t         length) {
  /*
   * NOTE: currently only trampoline and text region are executable,
   * and they are at the beginning of the address space, so this code
   * is fine.  We will probably never allow users to mark other pages
   * as executable; but if so, we will have to revisit how this check
   * is implemented.
   *
   * nap->static_text_end is a multiple of 4K, the memory protection
   * granularity.  Since this routine is used for checking whether
   * memory map adjustments / allocations -- which has 64K granularity
   * -- is okay, usraddr must be an allocation granularity value.  Our
   * callers (as of this writing) does this, but we truncate it down
   * to an allocation boundary to be sure.
   */
  UNREFERENCED_PARAMETER(length);
  usraddr = NaClTruncAllocPage(usraddr);
  return usraddr < nap->dynamic_text_end;
}


/* Warning: sizeof(nacl_abi_off_t)!=sizeof(off_t) on OSX */
int32_t NaClCommonSysMmapIntern(struct NaClApp        *nap,
                                void                  *start,
                                size_t                length,
                                int                   prot,
                                int                   flags,
                                int                   d,
                                nacl_abi_off_t        offset) {
  int                         allowed_flags;
  struct NaClDesc             *ndp;
  uintptr_t                   usraddr;
  uintptr_t                   usrpage;
  uintptr_t                   sysaddr;
  uintptr_t                   endaddr;
  uintptr_t                   map_result;
  int                         holding_app_lock;
  struct NaClMemObj           *nmop;
  struct nacl_abi_stat        stbuf;
  size_t                      alloc_rounded_length;
  nacl_off64_t                file_size;
  nacl_off64_t                file_bytes;
  nacl_off64_t                host_rounded_file_bytes;
  size_t                      alloc_rounded_file_bytes;
  size_t                      start_of_inaccessible;

  holding_app_lock = 0;
  nmop = NULL;
  ndp = NULL;

  allowed_flags = (NACL_ABI_MAP_FIXED | NACL_ABI_MAP_SHARED
                   | NACL_ABI_MAP_PRIVATE | NACL_ABI_MAP_ANONYMOUS);

  usraddr = (uintptr_t) start;

  if (0 != (flags & ~allowed_flags)) {
    NaClLog(LOG_WARNING, "invalid mmap flags 0%o, ignoring extraneous bits\n",
            flags);
    flags &= allowed_flags;
  }

  if (0 != (flags & NACL_ABI_MAP_ANONYMOUS)) {
    /*
     * anonymous mmap, so backing store is just swap: no descriptor is
     * involved, and no memory object will be created to represent the
     * descriptor.
     */
    ndp = NULL;
  } else {
    ndp = NaClGetDesc(nap, d);
    if (NULL == ndp) {
      map_result = -NACL_ABI_EBADF;
      goto cleanup;
    }
  }

  /*
   * Starting address must be aligned to worst-case allocation
   * granularity.  (Windows.)
   */
  if (!NaClIsAllocPageMultiple(usraddr)) {
    NaClLog(2, "NaClSysMmap: address not allocation granularity aligned\n");
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * Offset should be non-negative (nacl_abi_off_t is signed).  This
   * condition is caught when the file is stat'd and checked, and
   * offset is ignored for anonymous mappings.
   */
  if (offset < 0) {
    NaClLog(1,  /* application bug */
            "NaClSysMmap: negative file offset: %"NACL_PRIdNACL_OFF"\n",
            offset);
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * And offset must be a multiple of the allocation unit.
   */
  if (!NaClIsAllocPageMultiple((uintptr_t) offset)) {
    NaClLog(1,
            ("NaClSysMmap: file offset 0x%08"NACL_PRIxPTR" not multiple"
             " of allocation size\n"),
            (uintptr_t) offset);
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  if (0 == length) {
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  alloc_rounded_length = NaClRoundAllocPage(length);
  if (alloc_rounded_length != length) {
    NaClLog(1,
            "mmap: rounded length to 0x%"NACL_PRIxS"\n",
            alloc_rounded_length);
  }

  if (NULL == ndp) {
    /*
     * Note: sentinel values are bigger than the NaCl module addr space.
     */
    file_size                = kMaxUsableFileSize;
    file_bytes               = kMaxUsableFileSize;
    host_rounded_file_bytes  = kMaxUsableFileSize;
    alloc_rounded_file_bytes = kMaxUsableFileSize;
  } else {
    /*
     * We stat the file to figure out its actual size.
     *
     * This is needed since we allow an app to mmap in a odd-sized
     * file and will zero fill the allocation page containing the last
     * byte(s) of the file, but if the app asked for a length that
     * goes beyond the last allocation page, that memory is actually
     * inaccessible.  Of course, the underlying OS deals with real
     * pages, and we may need to simulate this behavior (i.e., OSX and
     * Linux, we will need to put zero-filled pages between the last
     * 4K system page containing file data and the rest of the
     * simulated windows allocation 64K page.
     */
    map_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                  Fstat)(ndp, &stbuf);
    if (0 != map_result) {
      goto cleanup;
    }
    /*
     * BUG(bsy): there's a race between this fstat and the actual mmap
     * below.  It's probably insoluble.  Even if we fstat again after
     * mmap and compared, the mmap could have "seen" the file with a
     * different size, after which the racing thread restored back to
     * the same value before the 2nd fstat takes place.
     */
    file_size = stbuf.nacl_abi_st_size;

    if (file_size < offset) {
      map_result = -NACL_ABI_EINVAL;
      goto cleanup;
    }

    file_bytes = file_size - offset;
    NaClLog(4,
            "NaClCommonSysMmapIntern: file_bytes 0x%016"NACL_PRIxNACL_OFF"\n",
            file_bytes);
    if ((nacl_off64_t) kMaxUsableFileSize < file_bytes) {
      host_rounded_file_bytes = kMaxUsableFileSize;
    } else {
      host_rounded_file_bytes = NaClRoundHostAllocPage((size_t) file_bytes);
    }

    ASSERT(host_rounded_file_bytes <= (nacl_off64_t) kMaxUsableFileSize);
    /*
     * We need to deal with NaClRoundHostAllocPage rounding up to zero
     * from ~0u - n, where n < 4096 or 65536 (== 1 alloc page).
     *
     * Luckily, file_bytes is at most kMaxUsableFileSize which is
     * smaller than SIZE_T_MAX, so it should never happen, but we
     * leave the explicit check below as defensive programming.
     */
    alloc_rounded_file_bytes =
      NaClRoundAllocPage((size_t) host_rounded_file_bytes);

    if (0 == alloc_rounded_file_bytes && 0 != host_rounded_file_bytes) {
      map_result = -NACL_ABI_ENOMEM;
      goto cleanup;
    }

    /*
     * NB: host_rounded_file_bytes and alloc_rounded_file_bytes can be
     * zero.  Such an mmap just makes memory (offset relative to
     * usraddr) in the range [0, alloc_rounded_length) inaccessible.
     */
  }

  /*
   * host_rounded_file_bytes is how many bytes we can map from the
   * file, given the user-supplied starting offset.  It is at least
   * one page.  If it came from a real file, it is a multiple of
   * host-OS allocation size.  it cannot be larger than
   * kMaxUsableFileSize.
   */
  length = size_min(alloc_rounded_length, (size_t) host_rounded_file_bytes);
  start_of_inaccessible = size_min(alloc_rounded_length,
                                   alloc_rounded_file_bytes);

  /*
   * Now, we map, relative to usraddr, bytes [0, length) from the file
   * starting at offset, zero-filled pages for the memory region
   * [length, start_of_inaccessible), and inaccessible pages for the
   * memory region [start_of_inaccessible, alloc_rounded_length).
   */

  /*
   * Lock the addr space.
   */
  NaClXMutexLock(&nap->mu);

  while (0 != nap->threads_launching) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  nap->vm_hole_may_exist = 1;

  holding_app_lock = 1;

  if (0 == (flags & NACL_ABI_MAP_FIXED)) {
    /*
     * The user wants us to pick an address range.
     */
    if (0 == usraddr) {
      /*
       * Pick a hole in addr space of appropriate size, anywhere.
       * We pick one that's best for the system.
       */
      usrpage = NaClVmmapFindMapSpace(&nap->mem_map,
                                      alloc_rounded_length >> NACL_PAGESHIFT);
      NaClLog(4, "NaClSysMmap: FindMapSpace: page 0x%05"NACL_PRIxPTR"\n",
              usrpage);
      if (0 == usrpage) {
        map_result = -NACL_ABI_ENOMEM;
        goto cleanup;
      }
      usraddr = usrpage << NACL_PAGESHIFT;
      NaClLog(4, "NaClSysMmap: new starting addr: 0x%08"NACL_PRIxPTR
              "\n", usraddr);
    } else {
      /*
       * user supplied an addr, but it's to be treated as a hint; we
       * find a hole of the right size in the app's address space,
       * according to the usual mmap semantics.
       */
      usrpage = NaClVmmapFindMapSpaceAboveHint(&nap->mem_map,
                                               usraddr,
                                               (alloc_rounded_length
                                                >> NACL_PAGESHIFT));
      NaClLog(4, "NaClSysMmap: FindSpaceAboveHint: page 0x%05"NACL_PRIxPTR"\n",
              usrpage);
      if (0 == usrpage) {
        NaClLog(4, "NaClSysMmap: hint failed, doing generic allocation\n");
        usrpage = NaClVmmapFindMapSpace(&nap->mem_map,
                                        alloc_rounded_length >> NACL_PAGESHIFT);
      }
      if (0 == usrpage) {
        map_result = -NACL_ABI_ENOMEM;
        goto cleanup;
      }
      usraddr = usrpage << NACL_PAGESHIFT;
      NaClLog(4, "NaClSysMmap: new starting addr: 0x%08"NACL_PRIxPTR"\n",
              usraddr);
    }
  }

  /*
   * Validate [usraddr, endaddr) is okay.
   */
  if (usraddr >= ((uintptr_t) 1 << nap->addr_bits)) {
    NaClLog(2,
            ("NaClSysMmap: start address (0x%08"NACL_PRIxPTR") outside address"
             " space\n"),
            usraddr);
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  endaddr = usraddr + alloc_rounded_length;
  if (endaddr < usraddr) {
    NaClLog(0,
            ("NaClSysMmap: integer overflow -- "
             "NaClSysMmap(0x%08"NACL_PRIxPTR",0x%"NACL_PRIxS",0x%x,0x%x,%d,"
             "0x%08"NACL_PRIxPTR"\n"),
            usraddr, length, prot, flags, d, (uintptr_t) offset);
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  /*
   * NB: we use > instead of >= here.
   *
   * endaddr is the address of the first byte beyond the target region
   * and it can equal the address space limit.  (of course, normally
   * the main thread's stack is there.)
   */
  if (endaddr > ((uintptr_t) 1 << nap->addr_bits)) {
    NaClLog(2,
            ("NaClSysMmap: end address (0x%08"NACL_PRIxPTR") is beyond"
             " the end of the address space\n"),
            endaddr);
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  if (NaClSysCommonAddrRangeContainsExecutablePages_mu(nap,
                                                       usraddr,
                                                       length)) {
    NaClLog(2, "NaClSysMmap: region contains executable pages\n");
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  /*
   * Force NACL_ABI_MAP_FIXED, since we are specifying address in NaCl
   * app address space.
   */
  flags |= NACL_ABI_MAP_FIXED;

  /*
   * Never allow users to say that mmapped pages are executable.  This
   * is primarily for the service runtime's own bookkeeping -- prot is
   * used in NaClCommonUtilUpdateAddrMap -- since %cs restriction
   * makes page protection irrelevant, it doesn't matter that on many
   * systems (w/o NX) PROT_READ implies PROT_EXEC.
   */
  prot &= ~NACL_ABI_PROT_EXEC;

  /*
   * Exactly one of NACL_ABI_MAP_SHARED and NACL_ABI_MAP_PRIVATE is set.
   */
  if ((0 == (flags & NACL_ABI_MAP_SHARED))
      == (0 == (flags & NACL_ABI_MAP_PRIVATE))) {
    map_result = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  sysaddr = NaClUserToSys(nap, usraddr);

  /* [0, length) */
  if (length > 0) {
    if (NULL == ndp) {
      NaClLog(4,
              ("NaClSysMmap: NaClDescIoDescMap(,,0x%08"NACL_PRIxPTR","
               "0x%08"NACL_PRIxS",0x%x,0x%x,0x%08"NACL_PRIxPTR")\n"),
              sysaddr, length, prot, flags, (uintptr_t) offset);
      map_result = NaClDescIoDescMapAnon(nap->effp,
                                         (void *) sysaddr,
                                         length,
                                         prot,
                                         flags,
                                         (off_t) offset);
    } else {
      /*
       * This is a fix for Windows, where we cannot pass a size that
       * goes beyond the non-page-rounded end of the file.
       */
      size_t length_to_map = size_min(length, (size_t) file_bytes);

      NaClLog(4,
              ("NaClSysMmap: (*ndp->Map)(,,0x%08"NACL_PRIxPTR","
               "0x%08"NACL_PRIxS",0x%x,0x%x,0x%08"NACL_PRIxPTR")\n"),
              sysaddr, length, prot, flags, (uintptr_t) offset);

      map_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
                    Map)(ndp,
                         nap->effp,
                         (void *) sysaddr,
                         length_to_map,
                         prot,
                         flags,
                         (off_t) offset);
    }
    /*
     * "Small" negative integers are errno values.  Larger ones are
     * virtual addresses.
     */
    if (NaClPtrIsNegErrno(&map_result)) {
      NaClLog(LOG_FATAL,
              ("NaClSysMmap: Map failed, but we"
               " cannot handle address space move, error %"NACL_PRIuS"\n"),
              (size_t) map_result);
    }
    if (map_result != sysaddr) {
      NaClLog(LOG_FATAL, "system mmap did not honor NACL_ABI_MAP_FIXED\n");
    }
    if (prot == NACL_ABI_PROT_NONE) {
      /*
       * windows nacl_host_desc implementation requires that PROT_NONE
       * memory be freed using VirtualFree rather than
       * UnmapViewOfFile.  TODO(bsy): remove this ugliness.
       */
      NaClCommonUtilUpdateAddrMap(nap, sysaddr, length, PROT_NONE,
                                  NULL, file_size, offset, 0);
    } else {
      /* record change for file-backed memory */
      NaClCommonUtilUpdateAddrMap(nap, sysaddr, length, NaClProtMap(prot),
                                  ndp, file_size, offset, 0);
    }
  } else {
    map_result = sysaddr;
  }
  /* zero fill [length, start_of_inaccessible) */
  if (length < start_of_inaccessible) {
    size_t  map_len = start_of_inaccessible - length;

    NaClLog(2,
            ("zero-filling pages for memory range"
             " [0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR
             "), length 0x%"NACL_PRIxS"\n"),
            sysaddr + length, sysaddr + start_of_inaccessible, map_len);
    map_result = NaClHostDescMap((struct NaClHostDesc *) NULL,
                                 (void *) (sysaddr + length),
                                 map_len,
                                 prot,
                                 NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                                 (off_t) 0);
    if (NaClPtrIsNegErrno(&map_result)) {
      NaClLog(LOG_ERROR,
              ("Could not create zero-filled pages for memory range"
               " [0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR")\n"),
              sysaddr + length, sysaddr + start_of_inaccessible);
      goto cleanup;
    }
    NaClCommonUtilUpdateAddrMap(nap, sysaddr + length, map_len,
                                NaClProtMap(prot),
                                (struct NaClDesc *) NULL, 0, (off_t) 0, 0);
  }
  /* inaccessible: [start_of_inaccessible, alloc_rounded_length) */
  if (start_of_inaccessible < alloc_rounded_length) {
    size_t  map_len = alloc_rounded_length - start_of_inaccessible;

    NaClLog(2,
            ("inaccessible pages for memory range"
             " [0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR"),"
             " length 0x%"NACL_PRIxS"\n"),
            sysaddr + start_of_inaccessible,
            sysaddr + alloc_rounded_length,
            map_len);
    map_result = NaClHostDescMap((struct NaClHostDesc *) NULL,
                             (void *) (sysaddr + start_of_inaccessible),
                             map_len,
                             NACL_ABI_PROT_NONE,
                             NACL_ABI_MAP_ANONYMOUS | NACL_ABI_MAP_PRIVATE,
                             (off_t) 0);
    if (NaClPtrIsNegErrno(&map_result)) {
      NaClLog(LOG_ERROR,
            ("Could not create inaccessible pages for memory range"
             " [0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxPTR
             "), length 0x%"NACL_PRIxS"\n"),
            sysaddr + start_of_inaccessible,
            sysaddr + alloc_rounded_length,
            map_len);
    }
    NaClCommonUtilUpdateAddrMap(nap, sysaddr + start_of_inaccessible,
                                map_len, PROT_NONE,
                                (struct NaClDesc *) NULL, 0,
                                (off_t) 0, 0);
  }
  NaClLog(3, "NaClSysMmap: got address 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) map_result);

  map_result = usraddr;

cleanup:
  if (holding_app_lock) {
    nap->vm_hole_may_exist = 0;
    NaClXCondVarBroadcast(&nap->cv);
    NaClXMutexUnlock(&nap->mu);
  }
  if (NULL != ndp) {
    NaClDescUnref(ndp);
  }
  if (NaClPtrIsNegErrno(&map_result)) {
    free(nmop);
  }

  /*
   * Check to ensure that map_result will fit into a 32-bit value. This is
   * a bit tricky because there are two valid ranges: one is the range from
   * 0 to (almost) 2^32, the other is from -1 to -4096 (our error range).
   * For a 32-bit value these ranges would overlap, but if the value is 64-bit
   * they will be disjoint.
   */
  if (map_result > UINT32_MAX
      && !NaClPtrIsNegErrno(&map_result)) {
    NaClLog(LOG_FATAL, "Overflow in NaClSysMmap: return address is "
                       "0x%"NACL_PRIxPTR"\n", map_result);
  }
  NaClLog(3, "NaClSysMmap: returning 0x%08"NACL_PRIxPTR"\n", map_result);

  return (int32_t) map_result;
}

int32_t NaClCommonSysMmap(struct NaClAppThread  *natp,
                          void                  *start,
                          size_t                length,
                          int                   prot,
                          int                   flags,
                          int                   d,
                          nacl_abi_off_t        *offp) {
  int32_t         retval;
  uintptr_t       sysaddr;
  nacl_abi_off_t  offset;

  NaClLog(3,
          "Entered NaClSysMmap(0x%08"NACL_PRIxPTR",0x%"NACL_PRIxS","
          "0x%x,0x%x,%d,0x%08"NACL_PRIxPTR")\n",
          (uintptr_t) start, length, prot, flags, d, (uintptr_t) offp);

  if ((nacl_abi_off_t *) 0 == offp) {
    /*
     * This warning is really targetted towards trusted code,
     * especially tests that didn't notice the argument type change.
     * Unfortunatey, zero is a common and legitimate offset value, and
     * the compiler will not complain since an automatic type
     * conversion works.
     */
    NaClLog(LOG_WARNING,
            "NaClCommonSysMmap: NULL pointer used"
            " for offset in/out argument\n");
    return -NACL_ABI_EINVAL;
  }

  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) offp, sizeof offset);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(3,
            "NaClCommonSysMmap: offset in a bad untrusted memory location\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  offset = *(nacl_abi_off_t volatile *) sysaddr;

  NaClLog(4, " offset = 0x%08"NACL_PRIxNACL_OFF"\n", offset);

  retval = NaClCommonSysMmapIntern(natp->nap,
                                   start, length,
                                   prot,
                                   flags,
                                   d, offset);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysImc_MakeBoundSock(struct NaClAppThread *natp,
                                       int32_t              *sap) {
  /*
   * Create a bound socket descriptor and a socket address descriptor.
   */

  int32_t                     retval = -NACL_ABI_EINVAL;
  uintptr_t                   sys_sap;
  struct NaClDesc             *pair[2];

  NaClLog(3,
          ("Entered NaClCommonSysImc_MakeBoundSock(0x%08"NACL_PRIxPTR","
           " 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) sap);

  NaClSysCommonThreadSyscallEnter(natp);

  sys_sap = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) sap,
                                   2 * sizeof *sap);
  if (kNaClBadAddress == sys_sap) {
    NaClLog(3, " illegal address\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  retval = NaClCommonDescMakeBoundSock(pair);
  if (0 != retval) {
    goto cleanup;
  }

  ((int32_t *) sys_sap)[0] = NaClSetAvail(natp->nap, pair[0]);
  ((int32_t *) sys_sap)[1] = NaClSetAvail(natp->nap, pair[1]);
  retval = 0;
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysImc_Accept(struct NaClAppThread  *natp,
                                int                   d) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysImc_Accept(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
  } else {
    struct NaClDesc *result_desc;
    retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              AcceptConn)(ndp, &result_desc);
    if (retval == 0) {
      retval = NaClSetAvail(natp->nap, result_desc);
    }
    NaClDescUnref(ndp);
  }

  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysImc_Connect(struct NaClAppThread *natp,
                                 int                  d) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *ndp;

  NaClLog(3, "Entered NaClSysImc_ConnectAddr(0x%08"NACL_PRIxPTR", %d)\n",
          (uintptr_t) natp, d);

  NaClSysCommonThreadSyscallEnter(natp);

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
  } else {
    struct NaClDesc *result;
    retval = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->
              ConnectAddr)(ndp, &result);
    if (retval == 0) {
      retval = NaClSetAvail(natp->nap, result);
    }
    NaClDescUnref(ndp);
  }

  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

/*
 * This function converts addresses from user addresses to system
 * addresses, copying into kernel space as needed to avoid TOCvTOU
 * races, then invoke NaClImcSendTypedMessage from the nrd_xfer
 * library.
 */
int32_t NaClCommonSysImc_Sendmsg(struct NaClAppThread         *natp,
                                 int                          d,
                                 struct NaClAbiNaClImcMsgHdr *nanimhp,
                                 int                          flags) {
  int32_t                       retval = -NACL_ABI_EINVAL;
  ssize_t                       ssize_retval;
  uintptr_t                     sysaddr;
  /* copy of user-space data for validation */
  struct NaClAbiNaClImcMsgHdr   kern_nanimh;
  struct NaClAbiNaClImcMsgIoVec kern_naiov[NACL_ABI_IMC_IOVEC_MAX];
  struct NaClImcMsgIoVec        kern_iov[NACL_ABI_IMC_IOVEC_MAX];
  /* kernel-side representatin of descriptors */
  struct NaClDesc               *kern_desc[NACL_ABI_IMC_USER_DESC_MAX];
  struct NaClImcTypedMsgHdr     kern_msg_hdr;
  struct NaClDesc               *ndp;
  size_t                        i;

  NaClLog(3,
          ("Entered NaClCommonSysImc_Sendmsg(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIxPTR", 0x%x)\n"),
          (uintptr_t) natp, d, (uintptr_t) nanimhp, flags);

  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) nanimhp,
                                   sizeof *nanimhp);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "NaClImcMsgHdr not in user address space\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_leave;
  }

  kern_nanimh = *(struct NaClAbiNaClImcMsgHdr volatile *) sysaddr;
  /* copy before validating */

  /*
   * Some of these checks duplicate checks that will be done in the
   * nrd xfer library, but it is better to check before doing the
   * address translation of memory/descriptor vectors if those vectors
   * might be too long.  Plus, we need to copy and validate vectors
   * for TOCvTOU race protection, and we must prevent overflows.  The
   * nrd xfer library's checks should never fire when called from the
   * service runtime, but the nrd xfer library might be called from
   * other code.
   */
  if (kern_nanimh.iov_length > NACL_ABI_IMC_IOVEC_MAX) {
    NaClLog(4, "gather/scatter array too large\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }
  if (kern_nanimh.desc_length > NACL_ABI_IMC_USER_DESC_MAX) {
    NaClLog(4, "handle vector too long\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }

  if (kern_nanimh.iov_length > 0) {
    sysaddr = NaClUserToSysAddrRange(natp->nap,
                                     (uintptr_t) kern_nanimh.iov,
                                     (kern_nanimh.iov_length
                                      * sizeof kern_naiov[0]));
    if (kNaClBadAddress == sysaddr) {
      NaClLog(4, "gather/scatter array not in user address space\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }

    memcpy(kern_naiov, (void *) sysaddr,
           kern_nanimh.iov_length * sizeof kern_naiov[0]);

    for (i = 0; i < kern_nanimh.iov_length; ++i) {
      sysaddr = NaClUserToSysAddrRange(natp->nap,
                                       (uintptr_t) kern_naiov[i].base,
                                       kern_naiov[i].length);
      if (kNaClBadAddress == sysaddr) {
        retval = -NACL_ABI_EFAULT;
        goto cleanup_leave;
      }
      kern_iov[i].base = (void *) sysaddr;
      kern_iov[i].length = kern_naiov[i].length;
    }
  }

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    retval = -NACL_ABI_EBADF;
    goto cleanup_leave;
  }

  /*
   * make things easier for cleaup exit processing
   */
  memset(kern_desc, 0, sizeof kern_desc);
  retval = -NACL_ABI_EINVAL;

  kern_msg_hdr.iov = kern_iov;
  kern_msg_hdr.iov_length = kern_nanimh.iov_length;

  if (0 == kern_nanimh.desc_length) {
    kern_msg_hdr.ndescv = 0;
    kern_msg_hdr.ndesc_length = 0;
  } else {
    sysaddr = NaClUserToSysAddrRange(natp->nap,
                                     (uintptr_t) kern_nanimh.descv,
                                     kern_nanimh.desc_length * sizeof(int32_t));
    if (kNaClBadAddress == sysaddr) {
      retval = -NACL_ABI_EFAULT;
      goto cleanup;
    }

    /*
     * NB: for each descv entry, we read from NaCl app address space
     * exactly once.
     */
    for (i = 0; i < kern_nanimh.desc_length; ++i) {
      int32_t user_desc = ((volatile int32_t *) sysaddr)[i];
      /* fetch it once */

      if (kKnownInvalidDescNumber == user_desc) {
        kern_desc[i] = (struct NaClDesc *) NaClDescInvalidMake();
      } else {
        /* NaCl modules are ILP32, so this works on ILP32 and LP64 systems */
        kern_desc[i] = NaClGetDesc(natp->nap, user_desc);
      }
      if (NULL == kern_desc[i]) {
        retval = -NACL_ABI_EBADF;
        goto cleanup;
      }
    }
    kern_msg_hdr.ndescv = kern_desc;
    kern_msg_hdr.ndesc_length = kern_nanimh.desc_length;
  }
  kern_msg_hdr.flags = kern_nanimh.flags;

  ssize_retval = NaClImcSendTypedMessage(ndp, &kern_msg_hdr, flags);

  if (NaClSSizeIsNegErrno(&ssize_retval)) {
    /*
     * NaClWouldBlock uses TSD (for both the errno-based and
     * GetLastError()-based implementations), so this is threadsafe.
     */
    if (0 != (flags & NACL_DONT_WAIT) && NaClWouldBlock()) {
      retval = -NACL_ABI_EAGAIN;
    } else if (-NACL_ABI_EMSGSIZE == ssize_retval) {
      /*
       * Allow the caller to handle the case when imc_sendmsg fails because
       * the message is too large for the system to send in one piece.
       */
      retval = -NACL_ABI_EMSGSIZE;
    } else {
      /*
       * TODO(bsy): the else case is some mysterious internal error.
       * Should we destroy the ndp or otherwise mark it as bad?  Was
       * the failure atomic?  Did it send some partial data?  Linux
       * implementation appears okay.
       */
      retval = -NACL_ABI_EIO;
    }
  } else if (ssize_retval > INT32_MAX || ssize_retval < INT32_MIN) {
    retval = -NACL_ABI_EOVERFLOW;
  } else {
    /* cast is safe due to range checks above */
    retval = (int32_t)ssize_retval;
  }

cleanup:
  for (i = 0; i < kern_nanimh.desc_length; ++i) {
    if (NULL != kern_desc[i]) {
      NaClDescUnref(kern_desc[i]);
      kern_desc[i] = NULL;
    }
  }
  NaClDescUnref(ndp);
cleanup_leave:
  NaClSysCommonThreadSyscallLeave(natp);
  NaClLog(3, "NaClCommonSysImc_Sendmsg: returning %d\n", retval);
  return retval;
}

int32_t NaClCommonSysImc_Recvmsg(struct NaClAppThread         *natp,
                                 int                          d,
                                 struct NaClAbiNaClImcMsgHdr  *nanimhp,
                                 int                          flags) {
  int32_t                               retval = -NACL_ABI_EINVAL;
  ssize_t                               ssize_retval;
  uintptr_t                             sysaddr;
  struct NaClAbiNaClImcMsgHdr volatile  *kern_nanimhp;
  size_t                                i;
  struct NaClDesc                       *ndp;
  struct NaClAbiNaClImcMsgHdr           kern_nanimh;
  struct NaClAbiNaClImcMsgIoVec         kern_naiov[NACL_ABI_IMC_IOVEC_MAX];
  struct NaClImcMsgIoVec                kern_iov[NACL_ABI_IMC_IOVEC_MAX];
  int32_t volatile                      *kern_descv;
  struct NaClImcTypedMsgHdr             recv_hdr;
  struct NaClDesc                       *new_desc[NACL_ABI_IMC_DESC_MAX];
  nacl_abi_size_t                       num_user_desc;
  struct NaClDesc                       *invalid_desc = NULL;

  NaClLog(3,
          ("Entered NaClCommonSysImc_RecvMsg(0x%08"NACL_PRIxPTR", %d,"
           " 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, d, (uintptr_t) nanimhp);

  NaClSysCommonThreadSyscallEnter(natp);

  /*
   * First, we validate user-supplied message headers before
   * allocating a receive buffer.
   */
  sysaddr = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) nanimhp,
                                   sizeof *nanimhp);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "NaClImcMsgHdr not in user address space\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup_leave;
  }
  kern_nanimhp = (struct NaClAbiNaClImcMsgHdr volatile *) sysaddr;
  kern_nanimh = *kern_nanimhp;
  /* copy before validating */

  if (kern_nanimh.iov_length > NACL_ABI_IMC_IOVEC_MAX) {
    NaClLog(4, "gather/scatter array too large: %"NACL_PRIdNACL_SIZE"\n",
            kern_nanimh.iov_length);
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }
  if (kern_nanimh.desc_length > NACL_ABI_IMC_USER_DESC_MAX) {
    NaClLog(4, "handle vector too long: %"NACL_PRIdNACL_SIZE"\n",
            kern_nanimh.desc_length);
    retval = -NACL_ABI_EINVAL;
    goto cleanup_leave;
  }

  if (kern_nanimh.iov_length > 0) {
    sysaddr = NaClUserToSysAddrRange(natp->nap,
                                     (uintptr_t) kern_nanimh.iov,
                                     (kern_nanimh.iov_length
                                      * sizeof kern_naiov[0]));
    if (kNaClBadAddress == sysaddr) {
      NaClLog(4, "gather/scatter array not in user address space\n");
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }
    /*
     * Copy IOV array into kernel space.  Validate this snapshot and do
     * user->kernel address conversions on this snapshot.
     */
    memcpy(kern_naiov, (void *) sysaddr,
           kern_nanimh.iov_length * sizeof kern_naiov[0]);
    /*
     * Convert every IOV base from user to system address, validate
     * range of bytes are really in user address space.
     */

    for (i = 0; i < kern_nanimh.iov_length; ++i) {
      sysaddr = NaClUserToSysAddrRange(natp->nap,
                                       (uintptr_t) kern_naiov[i].base,
                                       kern_naiov[i].length);
      if (kNaClBadAddress == sysaddr) {
        NaClLog(4, "iov number %"NACL_PRIdS" not entirely in user space\n", i);
        retval = -NACL_ABI_EFAULT;
        goto cleanup_leave;
      }
      kern_iov[i].base = (void *) sysaddr;
      kern_iov[i].length = kern_naiov[i].length;
    }
  }

  if (kern_nanimh.desc_length > 0) {
    sysaddr = NaClUserToSysAddrRange(natp->nap,
                                     (uintptr_t) kern_nanimh.descv,
                                     kern_nanimh.desc_length * sizeof(int32_t));
    if (kNaClBadAddress == sysaddr) {
      retval = -NACL_ABI_EFAULT;
      goto cleanup_leave;
    }
    kern_descv = (int32_t volatile *) sysaddr;
  } else {
    /* ensure we will SEGV if there's a bug below */
    kern_descv = (int32_t volatile *) NULL;
  }

  ndp = NaClGetDesc(natp->nap, d);
  if (NULL == ndp) {
    NaClLog(4, "receiving descriptor invalid\n");
    retval = -NACL_ABI_EBADF;
    goto cleanup_leave;
  }

  recv_hdr.iov = kern_iov;
  recv_hdr.iov_length = kern_nanimh.iov_length;

  recv_hdr.ndescv = new_desc;
  recv_hdr.ndesc_length = NACL_ARRAY_SIZE(new_desc);
  memset(new_desc, 0, sizeof new_desc);

  recv_hdr.flags = 0;  /* just to make it obvious; IMC will clear it for us */

  ssize_retval = NaClImcRecvTypedMessage(ndp, &recv_hdr, flags);
  /*
   * retval is number of user payload bytes received and excludes the
   * header bytes.
   */
  NaClLog(3, "NaClCommonSysImc_RecvMsg: "
          "NaClImcRecvTypedMessage returned %"NACL_PRIdS"\n",
          ssize_retval);
  if (NaClSSizeIsNegErrno(&ssize_retval)) {
    /* negative error numbers all have valid 32-bit representations,
     * so this cast is safe. */
    retval = (int32_t) ssize_retval;
    goto cleanup;
  } else if (ssize_retval > INT32_MAX || ssize_retval < INT32_MIN) {
    retval = -NACL_ABI_EOVERFLOW;
    goto cleanup;
  } else {
    /* cast is safe due to range check above */
    retval = (int32_t) ssize_retval;
  }

  /*
   * NB: recv_hdr.flags may contain NACL_ABI_MESSAGE_TRUNCATED and/or
   * NACL_ABI_HANDLES_TRUNCATED.
   */

  kern_nanimh.flags = recv_hdr.flags;

  /*
   * Now internalize the NaClHandles as NaClDesc objects.
   */
  num_user_desc = recv_hdr.ndesc_length;

  if (kern_nanimh.desc_length < num_user_desc) {
    kern_nanimh.flags |= NACL_ABI_RECVMSG_DESC_TRUNCATED;
    for (i = kern_nanimh.desc_length; i < num_user_desc; ++i) {
      NaClDescUnref(new_desc[i]);
      new_desc[i] = NULL;
    }
    num_user_desc = kern_nanimh.desc_length;
  }

  invalid_desc = (struct NaClDesc *) NaClDescInvalidMake();
  for (i = 0; i < num_user_desc; ++i) {
    /* write out to user space the descriptor numbers */
    if (invalid_desc == new_desc[i]) {
      kern_descv[i] = kKnownInvalidDescNumber;
    } else {
      kern_descv[i] = NaClSetAvail(natp->nap, new_desc[i]);
    }
    new_desc[i] = NULL;
  }

  kern_nanimh.desc_length = num_user_desc;
  *kern_nanimhp = kern_nanimh;
  /* copy out updated desc count, flags */
 cleanup:
  if (retval < 0) {
    for (i = 0; i < NACL_ARRAY_SIZE(new_desc); ++i) {
      if (NULL != new_desc[i]) {
        NaClDescUnref(new_desc[i]);
        new_desc[i] = NULL;
      }
    }
  }
  NaClDescUnref(ndp);
  NaClDescSafeUnref(invalid_desc);
  NaClLog(3, "NaClCommonSysImc_RecvMsg: returning %d\n", retval);
cleanup_leave:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysImc_Mem_Obj_Create(struct NaClAppThread  *natp,
                                        size_t                size) {
  int32_t               retval = -NACL_ABI_EINVAL;
  struct NaClDescImcShm *shmp;
  off_t                 size_as_off;

  NaClLog(3,
          ("Entered NaClCommonSysImc_Mem_Obj_Create(0x%08"NACL_PRIxPTR
           " 0x%08"NACL_PRIxS")\n"),
          (uintptr_t) natp, size);

  if (0 != (size & (NACL_MAP_PAGESIZE - 1))) {
    return -NACL_ABI_EINVAL;
  }
  /*
   * TODO(bsy): policy about maximum shm object size should be
   * enforced here.
   */
  size_as_off = (off_t) size;
  if (size_as_off < 0) {
    return -NACL_ABI_EINVAL;
  }

  shmp = NULL;

  NaClSysCommonThreadSyscallEnter(natp);

  shmp = malloc(sizeof *shmp);
  if (NULL == shmp) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  if (!NaClDescImcShmAllocCtor(shmp, size_as_off, /* executable= */ 0)) {
    retval = -NACL_ABI_ENOMEM;  /* is this reasonable? */
    goto cleanup;
  }

  retval = NaClSetAvail(natp->nap, (struct NaClDesc *) shmp);
  shmp = NULL;

cleanup:
  free(shmp);

  NaClSysCommonThreadSyscallLeave(natp);

  return retval;
}

int32_t NaClCommonSysImc_SocketPair(struct NaClAppThread *natp,
                                    int32_t volatile     *d_out) {
  uintptr_t               sysaddr;
  struct NaClDesc         *pair[2];
  int32_t                 retval;

  NaClLog(3,
          ("Entered NaClCommonSysImc_SocketPair(0x%08"NACL_PRIxPTR
           " 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) d_out);

  NaClSysCommonThreadSyscallEnter(natp);

  sysaddr = NaClUserToSysAddrRange(natp->nap,
                                   (uintptr_t) d_out,
                                   2 * sizeof *d_out);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(1,
            ("NaClCommonSysImc_Socket_Pair: bad output descriptor array "
             " (0x%08"NACL_PRIxPTR")\n"),
            (uintptr_t) d_out);
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  d_out = (int32_t volatile *) sysaddr;
  retval = NaClCommonDescSocketPair(pair);
  if (0 != retval) {
    goto cleanup;
  }

  d_out[0] = NaClSetAvail(natp->nap, pair[0]);
  d_out[1] = NaClSetAvail(natp->nap, pair[1]);

  retval = 0;
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysTls_Init(struct NaClAppThread  *natp,
                              void                  *thread_ptr) {
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sys_tls;

  NaClLog(3,
          ("Entered NaClCommonSysTls_Init(0x%08"NACL_PRIxPTR
           ", 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) thread_ptr);

  NaClSysCommonThreadSyscallEnter(natp);

  /* Verify that the address in the app's range and translated from
   * nacl module address to service runtime address - a nop on ARM
   */
  sys_tls = NaClUserToSysAddrRange(natp->nap, (uintptr_t) thread_ptr, 4);
  NaClLog(4,
          "NaClCommonSysTls_Init: thread_ptr 0x%p, sys_tls 0x%"NACL_PRIxPTR"\n",
          thread_ptr, sys_tls);
  if (kNaClBadAddress == sys_tls) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  if (0 == NaClTlsChange(natp, (void *) sys_tls)) {
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
  natp->sys_tls = sys_tls;
  retval = 0;
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysThread_Create(struct NaClAppThread *natp,
                                   void                 *prog_ctr,
                                   void                 *stack_ptr,
                                   void                 *thread_ptr,
                                   void                 *second_thread_ptr) {
  int32_t     retval = -NACL_ABI_EINVAL;
  uintptr_t   sys_tls;
  uintptr_t   sys_stack;

  NaClLog(3,
          ("Entered NaClCommonSysThread_Create(0x%08"NACL_PRIxPTR
           " pc=0x%08"NACL_PRIxPTR", sp=0x%08"NACL_PRIxPTR", thread_ptr=0x%08"
           NACL_PRIxPTR")\n"),
          (uintptr_t) natp, (uintptr_t) prog_ctr, (uintptr_t) stack_ptr,
          (uintptr_t) thread_ptr);

  NaClSysCommonThreadSyscallEnter(natp);

  /* make sure that the thread start function is in the text region */
  if ((uintptr_t) prog_ctr >= natp->nap->dynamic_text_end) {
    NaClLog(LOG_ERROR, "bad pc start\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  /* make sure that the thread start function is aligned */
  /* TODO(robertm): there should be a function for this test */

#if 0 == NACL_DANGEROUS_DEBUG_MODE_DISABLE_INNER_SANDBOX
  if (0 != ((natp->nap->bundle_size - 1) & (uintptr_t) prog_ctr)) {
    NaClLog(LOG_ERROR, "bad pc alignment\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }
#endif
  /* we do not enforce stack alignment, just check for validity */
  sys_stack = NaClUserToSysAddr(natp->nap, (uintptr_t) stack_ptr);
  if (kNaClBadAddress == sys_stack) {
    NaClLog(LOG_ERROR, "bad stack\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  sys_tls = NaClUserToSysAddrRange(natp->nap, (uintptr_t) thread_ptr, 4);
  if (kNaClBadAddress == sys_tls) {
    NaClLog(LOG_ERROR, "bad TLS pointer\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClVmHoleWaitToStartThread(natp->nap);

  retval = NaClCreateAdditionalThread(natp->nap,
                                      (uintptr_t) prog_ctr,
                                      sys_stack,
                                      sys_tls,
                                      (uint32_t) (uintptr_t) second_thread_ptr);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysTlsGet(struct NaClAppThread *natp) {
  uint32_t user_tls;

  /* too frequently used, and syscall-number level logging suffices */
  user_tls = (int32_t) NaClSysToUser(natp->nap, natp->sys_tls);
  return user_tls;
}

int NaClCommonSysThread_Nice(struct NaClAppThread *natp,
                             const int            nice) {
  /* Note: implementation of nacl_thread_nice is OS dependent. */
  UNREFERENCED_PARAMETER(natp);
  return nacl_thread_nice(nice);
}

int32_t NaClCommonSysMutex_Create(struct NaClAppThread *natp) {
  int32_t              retval = -NACL_ABI_EINVAL;
  struct NaClDescMutex *desc;

  NaClLog(3,
          ("Entered NaClCommonSysMutex_Create(0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescMutexCtor(desc)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClSetAvail(natp->nap, (struct NaClDesc *)desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClSysCommonThreadSyscallLeave(natp);
  NaClLog(3,
          ("NaClCommonSysMutex_Create(0x%08"NACL_PRIxPTR") = %d\n"),
          (uintptr_t) natp, retval);
  return retval;
}

int32_t NaClCommonSysMutex_Lock(struct NaClAppThread  *natp,
                                int32_t               mutex_handle) {
  int32_t               retval = -NACL_ABI_EINVAL;
  struct NaClDesc       *desc;

  NaClLog(3,
          ("Entered NaClCommonSysMutex_Lock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Lock)(desc);
  NaClDescUnref(desc);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysMutex_Unlock(struct NaClAppThread  *natp,
                                  int32_t               mutex_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysMutex_Unlock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Unlock)(desc);
  NaClDescUnref(desc);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysMutex_Trylock(struct NaClAppThread   *natp,
                                  int32_t                 mutex_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysMutex_Trylock(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, mutex_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, mutex_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->TryLock)(desc);
  NaClDescUnref(desc);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysCond_Create(struct NaClAppThread *natp) {
  int32_t                retval = -NACL_ABI_EINVAL;
  struct NaClDescCondVar *desc;

  NaClLog(3,
          ("Entered NaClCommonSysCond_Create(0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescCondVarCtor(desc)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClSetAvail(natp->nap, (struct NaClDesc *)desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClSysCommonThreadSyscallLeave(natp);
  NaClLog(3,
          ("NaClCommonSysCond_Create(0x%08"NACL_PRIxPTR") = %d\n"),
          (uintptr_t) natp, retval);
  return retval;
}

int32_t NaClCommonSysCond_Wait(struct NaClAppThread *natp,
                               int32_t              cond_handle,
                               int32_t              mutex_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *cv_desc;
  struct NaClDesc *mutex_desc;

  NaClLog(3,
          ("Entered NaClCommonSysCond_Wait(0x%08"NACL_PRIxPTR", %d, %d)\n"),
          (uintptr_t) natp, cond_handle, mutex_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  cv_desc = NaClGetDesc(natp->nap, cond_handle);

  if (NULL == cv_desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  mutex_desc = NaClGetDesc(natp->nap, mutex_handle);
  if (NULL == mutex_desc) {
    NaClDescUnref(cv_desc);
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) cv_desc->base.vtbl)->
            Wait)(cv_desc, mutex_desc);
  NaClDescUnref(cv_desc);
  NaClDescUnref(mutex_desc);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysCond_Signal(struct NaClAppThread *natp,
                                 int32_t              cond_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysCond_Signal(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, cond_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, cond_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Signal)(desc);
  NaClDescUnref(desc);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysCond_Broadcast(struct NaClAppThread  *natp,
                                    int32_t               cond_handle) {
  struct NaClDesc *desc;
  int32_t         retval = -NACL_ABI_EINVAL;

  NaClLog(3,
          ("Entered NaClCommonSysCond_Broadcast(0x%08"NACL_PRIxPTR", %d)\n"),
          (uintptr_t) natp, cond_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, cond_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Broadcast)(desc);
  NaClDescUnref(desc);

cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysCond_Timed_Wait_Abs(struct NaClAppThread     *natp,
                                         int32_t                  cond_handle,
                                         int32_t                  mutex_handle,
                                         struct nacl_abi_timespec *ts) {
  int32_t                  retval = -NACL_ABI_EINVAL;
  struct NaClDesc          *cv_desc;
  struct NaClDesc          *mutex_desc;
  uintptr_t                sys_ts;
  struct nacl_abi_timespec trusted_ts;

  NaClLog(3,
          ("Entered NaClCommonSysCond_Timed_Wait_Abs(0x%08"NACL_PRIxPTR
           ", %d, %d, 0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) natp, cond_handle, mutex_handle, (uintptr_t) ts);

  NaClSysCommonThreadSyscallEnter(natp);

  sys_ts = NaClUserToSysAddrRange(natp->nap,
                                  (uintptr_t) ts,
                                  sizeof(*ts));
  if (kNaClBadAddress == sys_ts) {
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  /* TODO(gregoryd): validate ts - do we have a limit for time to wait? */
  memcpy(&trusted_ts, (void *) sys_ts, sizeof(trusted_ts));

  cv_desc = NaClGetDesc(natp->nap, cond_handle);
  if (NULL == cv_desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  mutex_desc = NaClGetDesc(natp->nap, mutex_handle);
  if (NULL == mutex_desc) {
    NaClDescUnref(cv_desc);
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) cv_desc->base.vtbl)->
            TimedWaitAbs)(cv_desc,
                          mutex_desc,
                          &trusted_ts);
  NaClDescUnref(cv_desc);
  NaClDescUnref(mutex_desc);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysSem_Create(struct NaClAppThread *natp,
                                int32_t              init_value) {
  int32_t                  retval = -NACL_ABI_EINVAL;
  struct NaClDescSemaphore *desc;

  NaClLog(3,
          ("Entered NaClCommonSysSem_Create(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, init_value);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescSemaphoreCtor(desc, init_value)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClSetAvail(natp->nap, (struct NaClDesc *) desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}


int32_t NaClCommonSysSem_Wait(struct NaClAppThread *natp,
                              int32_t              sem_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysSem_Wait(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  /*
   * TODO(gregoryd): we have to decide on the syscall API: do we
   * switch to read/write/ioctl API or do we stay with the more
   * detailed API. Anyway, using a single syscall for waiting on all
   * synchronization objects makes sense.
   */
  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->SemWait)(desc);
  NaClDescUnref(desc);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysSem_Post(struct NaClAppThread *natp,
                              int32_t              sem_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysSem_Post(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->Post)(desc);
  NaClDescUnref(desc);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}

int32_t NaClCommonSysSem_Get_Value(struct NaClAppThread *natp,
                                   int32_t              sem_handle) {
  int32_t         retval = -NACL_ABI_EINVAL;
  struct NaClDesc *desc;

  NaClLog(3,
          ("Entered NaClCommonSysSem_Get_Value(0x%08"NACL_PRIxPTR
           ", %d)\n"),
          (uintptr_t) natp, sem_handle);

  NaClSysCommonThreadSyscallEnter(natp);

  desc = NaClGetDesc(natp->nap, sem_handle);

  if (NULL == desc) {
    retval = -NACL_ABI_EBADF;
    goto cleanup;
  }

  retval = (*((struct NaClDescVtbl const *) desc->base.vtbl)->GetValue)(desc);
  NaClDescUnref(desc);
cleanup:
  NaClSysCommonThreadSyscallLeave(natp);
  return retval;
}


int32_t NaClCommonSysTest_InfoLeak(struct NaClAppThread *natp) {
#if NACL_ARCH(NACL_BUILD_ARCH) == NACL_x86
  /*
   * Put some interesting bits into the x87 and SSE registers.
   */
  union fxsave {
    char buf[512];
    struct {
      uint16_t fcw;
      uint16_t fsw;
      uint16_t ftw;
      uint16_t fop;
      union {
        struct {
          uint64_t rip;
          uint64_t rdp;
        } x64;
        struct {
          uint32_t fpu_ip;
          uint32_t cs;
          uint32_t fpu_dp;
          uint32_t ds;
        } ia32;
      } bitness;
      uint32_t mxcsr;
      uint32_t mxcsr_mask;
      struct {
        uint8_t st[10];
        uint8_t reserved[6];
      } st_space[8];
      uint32_t xmm_space[64];
    } fxsave;
  };

  static const char tenbytes[10] = "SecretBits";
  static const char manybytes[256] =
      "Highly sensitive information must not be leaked to untrusted code!\n"
      "xyzzy\nplugh\nYou are likely to be eaten by a grue.\n"
      "When in the Course of human events it becomes necessary for one people"
      " to dissolve the political bands which have connected them with ...\n";

# ifdef __GNUC__
  union fxsave u __attribute__((aligned(16)));
# elif NACL_WINDOWS
  __declspec(align(16)) union fxsave u;
# else
#  error Unsupported platform
# endif

  int i;

# ifdef __GNUC__
  __asm__("fxsave %0" : "=m" (u));
# elif NACL_WINDOWS
#  if NACL_BUILD_SUBARCH == 64
  {
    void DoFxsave(union fxsave *);
    DoFxsave(&u);
  }
#  else
  __asm {
    fxsave u
  };
#  endif
# else
# error Unsupported platform
# endif

  for (i = 0; i < 8; ++i)
    memcpy(&u.fxsave.st_space[i], tenbytes, sizeof(tenbytes));

  memcpy(u.fxsave.xmm_space, manybytes, sizeof(u.fxsave.xmm_space));

# ifdef __GNUC__
  __asm__ volatile("fxrstor %0" :: "m" (u));
# elif NACL_WINDOWS
#  if NACL_BUILD_SUBARCH == 64
  {
    void DoFxrstor(union fxsave *);
    DoFxrstor(&u);
  }
#  else
  __asm {
    fxrstor u
  };
#  endif
# else
# error Unsupported platform
# endif

#endif

  UNREFERENCED_PARAMETER(natp);

  return -NACL_ABI_ENOSYS;
}
