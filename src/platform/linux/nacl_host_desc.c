/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 *
 * Note that we avoid using the thread-specific data / thread local
 * storage access to the "errno" variable, and instead use the raw
 * system call return interface of small negative numbers as errors.
 */

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include "src/platform/nacl_host_desc.h"
#include "src/platform/nacl_log.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/service_runtime/include/sys/fcntl.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/include/sys/stat.h"
//YaroslavLitvinov fixed error: undefined reference to S_ISREG,S_ISDIR
#include <sys/stat.h>

#include "src/service_runtime/sel_ldr.h" // ### to make gnap work
#include "src/service_runtime/nacl_globals.h" // ### to make gnap work
#include "src/manifest/manifest_setup.h" //### to make manifest work

/*
 * Map our ABI to the host OS's ABI.  On linux, this should be a big no-op.
 */
static INLINE int NaClMapOpenFlags(int nacl_flags) {
  int host_os_flags;

  nacl_flags &= (NACL_ABI_O_ACCMODE | NACL_ABI_O_CREAT
                 | NACL_ABI_O_TRUNC | NACL_ABI_O_APPEND);

  host_os_flags = 0;
#define C(H) case NACL_ABI_ ## H: \
  host_os_flags |= H;             \
  break;
  switch (nacl_flags & NACL_ABI_O_ACCMODE) {
    C(O_RDONLY);
    C(O_WRONLY);
    C(O_RDWR);
  }
#undef C
#define M(H) do { \
    if (0 != (nacl_flags & NACL_ABI_ ## H)) {   \
      host_os_flags |= H;                       \
    }                                           \
  } while (0)
  M(O_CREAT);
  M(O_TRUNC);
  M(O_APPEND);
#undef M
  return host_os_flags;
}

static INLINE int NaClMapOpenPerm(int nacl_perm) {
  int host_os_perm;

  host_os_perm = 0;
#define M(H) do { \
    if (0 != (nacl_perm & NACL_ABI_ ## H)) { \
      host_os_perm |= H; \
    } \
  } while (0)
  M(S_IRUSR);
  M(S_IWUSR);
#undef M
  return host_os_perm;
}

static INLINE int NaClMapFlagMap(int nacl_map_flags) {
  int host_os_flags;

  host_os_flags = 0;
#define M(H) do { \
    if (0 != (nacl_map_flags & NACL_ABI_ ## H)) { \
      host_os_flags |= H; \
    } \
  } while (0)
  M(MAP_SHARED);
  M(MAP_PRIVATE);
  M(MAP_FIXED);
  M(MAP_ANONYMOUS);
#undef M

  return host_os_flags;
}

/*
 * TODO(bsy): handle the !NACL_ABI_MAP_FIXED case.
 */
uintptr_t NaClHostDescMap(struct NaClHostDesc *d,
                          void                *start_addr,
                          size_t              len,
                          int                 prot,
                          int                 flags,
                          nacl_off64_t        offset) {
  int   desc;
  void  *map_addr;
  int   host_prot;
  int   host_flags;

  NaClLog(4,
          ("NaClHostDescMap(0x%08"NACL_PRIxPTR", "
           "0x%08"NACL_PRIxPTR", 0x%08"NACL_PRIxS", "
           "0x%x, 0x%x, 0x%08"NACL_PRIx64")\n"),
          (uintptr_t) d,
          (uintptr_t) start_addr,
          len,
          prot,
          flags,
          (int64_t) offset);
  if (NULL == d && 0 == (flags & NACL_ABI_MAP_ANONYMOUS)) {
    NaClLog(LOG_FATAL, "NaClHostDescMap: 'this' is NULL and not anon map\n");
  }
  prot &= (NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE);
  /* may be PROT_NONE too, just not PROT_EXEC */

  desc = flags & NACL_ABI_MAP_ANONYMOUS ? -1 : d->d;
  /*
   * Translate flags, prot to host_flags, host_prot.
   */
  host_flags = NaClMapFlagMap(flags);
  host_prot = NaClProtMap(prot);

  NaClLog(4, "NaClHostDescMap: host_flags 0x%x, host_prot 0x%x\n",
          host_flags, host_prot);

  /*
   * use "whole chunk" memory manager if set in manifest
   * and this is "user side call"
   * note: for safety we check (global) nap before use it
   */
  if(gnap != NULL && gnap->user_side_flag && gnap->manifest != NULL &&
      gnap->manifest->user_setup->max_mem)
  {
    map_addr = start_addr;
  }
  else
  {
    map_addr = mmap(start_addr, len, host_prot, host_flags, desc, offset);
  }

  if (MAP_FAILED == map_addr) {
    NaClLog(LOG_INFO,
            ("NaClHostDescMap: "
             "mmap(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS", "
             "0x%x, 0x%x, 0x%d, 0x%"NACL_PRIx64")"
             " failed, errno %d.\n"),
            (uintptr_t) start_addr, len, host_prot, host_flags, desc,
            (int64_t) offset,
            errno);
    return -NaClXlateErrno(errno);
  }
  if (0 != (flags & NACL_ABI_MAP_FIXED) && map_addr != start_addr) {
    NaClLog(LOG_FATAL,
            ("NaClHostDescMap: mmap with MAP_FIXED not fixed:"
             " returned 0x%08"NACL_PRIxPTR" instead of 0x%08"NACL_PRIxPTR"\n"),
            (uintptr_t) map_addr,
            (uintptr_t) start_addr);
  }
  NaClLog(4, "NaClHostDescMap: returning 0x%08"NACL_PRIxPTR"\n",
          (uintptr_t) start_addr);

  return (uintptr_t) start_addr;
}

int NaClHostDescUnmapUnsafe(void    *start_addr,
                            size_t  len) {
  int       retval;

  return ((-1 == (retval = munmap(start_addr, len)))
          ? -NaClXlateErrno(errno)
          : retval);
}

int NaClHostDescUnmap(void    *start_addr,
                      size_t  len) {
  int       retval;

  return ((-1 == (retval = (uintptr_t) mmap(start_addr, len,
                                            PROT_NONE,
                                            (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED),
                                            -1, (nacl_off64_t) 0)))
          ? -NaClXlateErrno(errno) : retval);
}

int NaClHostDescCtor(struct NaClHostDesc  *d,
                     int                  fd) {
  d->d = fd;
  NaClLog(3, "NaClHostDescCtor: success.\n");
  return 0;
}

int NaClHostDescOpen(struct NaClHostDesc  *d,
                     char const           *path,
                     int                  flags,
                     int                  mode) {
  int         host_desc;
  struct stat stbuf;

  NaClLog(3, "NaClHostDescOpen(0x%08"NACL_PRIxPTR", %s, 0x%x, 0x%x)\n",
          (uintptr_t) d, path, flags, mode);
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescOpen: 'this' is NULL\n");
  }
  /*
   * Sanitize access flags.
   */
  if (0 != (flags & ~NACL_ALLOWED_OPEN_FLAGS)) {
    return -NACL_ABI_EINVAL;
  }

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
    case NACL_ABI_O_WRONLY:
    case NACL_ABI_O_RDWR:
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescOpen: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  flags = NaClMapOpenFlags(flags);
  mode = NaClMapOpenPerm(mode);

  NaClLog(3, "NaClHostDescOpen: invoking POSIX open(%s,0x%x,0%o)\n",
          path, flags, mode);
  host_desc = open(path, flags, mode);
  NaClLog(3, "NaClHostDescOpen: got descriptor %d\n", host_desc);
  if (-1 == host_desc) {
    NaClLog(LOG_ERROR,
            "NaClHostDescOpen: open returned -1, errno %d\n", errno);
    return -NaClXlateErrno(errno);
  }
  if (-1 == fstat(host_desc, &stbuf)) {
    NaClLog(LOG_ERROR,
            "NaClHostDescOpen: fstat failed?!?  errno %d\n", errno);
    (void) close(host_desc);
    return -NaClXlateErrno(errno);
  }
  if (!S_ISREG(stbuf.st_mode)) {
    NaClLog(LOG_INFO,
            "NaClHostDescOpen: file type 0x%x, not regular\n", stbuf.st_mode);
    (void) close(host_desc);
    /* cannot access anything other than a real file */
    return -NACL_ABI_EPERM;
  }
  return NaClHostDescCtor(d, host_desc);
}

int NaClHostDescPosixTake(struct NaClHostDesc *d,
                          int                 posix_d,
                          int                 flags) {
  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescPosixTake: 'this' is NULL\n");
  }
  /*
   * Sanitize access flags.
   */
  if (0 != (flags & ~NACL_ALLOWED_OPEN_FLAGS)) {
    return -NACL_ABI_EINVAL;
  }

  switch (flags & NACL_ABI_O_ACCMODE) {
    case NACL_ABI_O_RDONLY:
    case NACL_ABI_O_WRONLY:
    case NACL_ABI_O_RDWR:
      break;
    default:
      NaClLog(LOG_ERROR,
              "NaClHostDescPosixTake: bad access flags 0x%x.\n",
              flags);
      return -NACL_ABI_EINVAL;
  }

  d->d = posix_d;
  return 0;
}

ssize_t NaClHostDescRead(struct NaClHostDesc  *d,
                         void                 *buf,
                         size_t               len) {
  ssize_t retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescRead: 'this' is NULL\n");
  }
  return ((-1 == (retval = read(d->d, buf, len)))
          ? -NaClXlateErrno(errno) : retval);
}

ssize_t NaClHostDescWrite(struct NaClHostDesc *d,
                          void const          *buf,
                          size_t              len) {
  ssize_t retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescWrite: 'this' is NULL\n");
  }
  return ((-1 == (retval = write(d->d, buf, len)))
          ? -NaClXlateErrno(errno) : retval);
}

nacl_off64_t NaClHostDescSeek(struct NaClHostDesc  *d,
                              nacl_off64_t         offset,
                              int                  whence)
{
  nacl_off64_t retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescSeek: 'this' is NULL\n");
  }
  return ((-1 == (retval = lseek64(d->d, offset, whence)))
          ? -NaClXlateErrno(errno) : retval);
}

int NaClHostDescIoctl(struct NaClHostDesc *d,
                      int                 request,
                      void                *arg)
{
  UNREFERENCED_PARAMETER(request);
  UNREFERENCED_PARAMETER(arg);

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescIoctl: 'this' is NULL\n");
  }
  return -NACL_ABI_ENOSYS;
}

int NaClHostDescFstat(struct NaClHostDesc  *d,
                      nacl_host_stat_t     *nhsp)
{
  if(fstat64(d->d, nhsp) == -1) return -errno;
  return 0;
}

int NaClHostDescClose(struct NaClHostDesc *d) {
  int retval;

  if (NULL == d) {
    NaClLog(LOG_FATAL, "NaClHostDescClose: 'this' is NULL\n");
  }
  retval = close(d->d);
  if (-1 != retval) {
    d->d = -1;
  }
  return (-1 == retval) ? -NaClXlateErrno(errno) : retval;
}
