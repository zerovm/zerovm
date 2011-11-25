/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl Service Runtime.  I/O Descriptor / Handle abstraction.  Memory
 * mapping using descriptors.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__

#include "native_client/src/include/portability.h"
#include "native_client/src/shared/platform/nacl_sync.h"

#if NACL_LINUX || NACL_OSX
# include "native_client/src/shared/platform/linux/nacl_host_desc_types.h"
#elif NACL_WINDOWS
# include "native_client/src/shared/platform/win/nacl_host_desc_types.h"
#endif



/*
 * see NACL_MAP_PAGESIZE from nacl_config.h; map operations must be aligned
 */

EXTERN_C_BEGIN

struct nacl_abi_stat;
struct NaClHostDesc;

/*
 * off64_t in linux, off_t in osx and __int64_t in win
 */
typedef int64_t nacl_off64_t;

/*
 * We do not explicitly provide an abstracted version of a
 * host-independent stat64 structure.  Instead, it is up to the user
 * of the nacl_host_desc code to not use anything but the
 * POSIX-blessed fields, to know that the shape/size may differ across
 * platforms, and to know that the st_size field is a 64-bit value
 * compatible w/ nacl_off64_t above.
 */
#if NACL_LINUX
typedef struct stat64 nacl_host_stat_t;
#elif NACL_OSX
typedef struct stat nacl_host_stat_t;
#elif NACL_WINDOWS
typedef struct _stati64 nacl_host_stat_t;
#elif defined __native_client__
/* nacl_host_stat_t not exposed to NaCl module code */
#else
# error "what OS?"
#endif

/* TODO(ilewis, bsy): it seems like these error functions are useful in more
 * places than just the NaClDesc library. Move them to a more central header.
 * When that's done, it should be possible to include this header *only* from
 * host code, removing the need for the #ifdef __native_client__ directives.
 *
 * Currently that's not possible, unless we want to forego NaClIsNegErrno in
 * trusted code, and go back to writing if(retval < 0) to check for errors.
 */

/*
 * On 64-bit Linux, the app has the entire 32-bit address space
 * (kernel no longer occupies the top 1G), so what is an errno and
 * what is an address is trickier: we require that our NACL_ABI_
 * errno values be at most 64K.
 *
 * NB: The runtime assumes that valid errno values can ALWAYS be
 * represented using no more than 16 bits. If this assumption
 * changes, all code dealing with error number should be reviewed.
 *
 * NB 2010-02-03: The original code for this function did not work:
 *   return ((uint64_t) val) >= ~((uint64_t) 0xffff);
 * Macintosh optimized builds were not able to recognize negative values.
 * All other platforms as well as Macintosh debug builds worked fine.
 *
 * NB the 3rd, 2010-10-19: these functions take pointer arguments
 * to discourage accidental use of narrowing/widening conversions,
 * which have caused problems in the past.  We assume without proof
 * that the compiler will do the right thing when inlining.
 */
static INLINE int NaClPtrIsNegErrno(const uintptr_t *val) {
  return (*val & ~((uintptr_t) 0xffff)) == ~((uintptr_t) 0xffff);
}

static INLINE int NaClSSizeIsNegErrno(const ssize_t *val) {
  return (*val & ~((ssize_t) 0xffff)) == ~((ssize_t) 0xffff);
}

static INLINE int NaClOff64IsNegErrno(const nacl_off64_t *val) {
  return (*val & ~((nacl_off64_t) 0xffff)) == ~((nacl_off64_t) 0xffff);
}

extern int NaClXlateErrno(int errnum);

extern int NaClXlateNaClSyncStatus(NaClSyncStatus status);

#ifndef __native_client__ /* these functions are not exposed to NaCl modules
                           * (see TODO comment above)
                           */
/*
 * Mapping data from a file.
 *
 * start_addr and len must be multiples of NACL_MAP_PAGESIZE.
 *
 * Of prot bits, only PROT_READ and PROT_WRITE are allowed.  Of flags,
 * only MAP_ANONYMOUS is allowed.  start_addr must be specified, and
 * this code will add in MAP_FIXED.  start_address, len, and offset
 * must be a multiple of NACL_MAP_PAGESIZE.
 *
 * Note that in Windows, in order for the mapping to be coherent, the
 * mapping must arise from the same mapping handle and just using the
 * same file handle won't ensure coherence.  If the file mapping
 * object were created and destroyed inside of NaClHostDescMap, there
 * would never be any chance at coherence.  One alternative is to
 * create a file mapping object for each mapping mode.  Native
 * descriptors are then shareable, but only when the mode matches (!).
 * A read-only shared mapping not seeing the changes made by a
 * read-write mapping seem rather ugly.
 *
 * Instead of this ugliness, we just say that a map operation cannot
 * request MAP_SHARED.  Anonymous mappings ignore the descriptor
 * argument.
 *
 * Underlying host-OS syscalls:  mmap / MapViewOfFileEx
 *
 * 4GB file max
 */
extern uintptr_t NaClHostDescMap(struct NaClHostDesc  *d,
                                 void                 *start_addr,
                                 size_t               len,
                                 int                  prot,
                                 int                  flags,
                                 nacl_off64_t         offset);

/*
 * Undo a file mapping.  The memory range specified by start_address,
 * len must be memory that came from NaClHostDescMap.
 *
 * start_addr and len must be multiples of NACL_MAP_PAGESIZE.
 *
 * Underlying host-OS syscalls: mmap / UnmapViewOfFile/VirtualAlloc
 */
extern int NaClHostDescUnmap(void   *start_addr,
                             size_t len);

/*
 * Undo a file mapping.  The memory range specified by start_address,
 * len must be memory that came from NaClHostDescMap.
 *
 * start_addr and len must be multiples of NACL_MAP_PAGESIZE.
 *
 * Underlying host-OS syscalls: munmap / UnmapViewOfFile
 */
extern int NaClHostDescUnmapUnsafe(void   *start_addr,
                                   size_t len);


/*
 * These are the flags that are permitted.
 */
#define NACL_ALLOWED_OPEN_FLAGS                 \
  (NACL_ABI_O_ACCMODE | NACL_ABI_O_CREAT        \
   | NACL_ABI_O_TRUNC | NACL_ABI_O_APPEND)

/*
 * Constructor for a NaClHostDesc object.
 *
 * path should be a host-OS pathname to a file.  No validation is
 * done.  flags should contain one of O_RDONLY, O_WRONLY, and O_RDWR,
 * and can additionally contain O_CREAT, O_TRUNC, and O_APPEND.
 *
 * Uses raw syscall return convention, so returns 0 for success and
 * non-zero (usually -NACL_ABI_EINVAL) for failure.
 *
 * We cannot return the platform descriptor/handle and be consistent
 * with a largely POSIX-ish interface, since e.g. windows handles may
 * be negative and might look like negative errno returns.  Currently
 * we use the posix API on windows, so it could work, but we may need
 * to change later.
 *
 * Underlying host-OS functions: open / _s_open_s
 */
extern int NaClHostDescOpen(struct NaClHostDesc *d,
                            char const          *path,
                            int                 flags,
                            int                 perms);

/*
 * Constructor for a NaClHostDesc object.
 *
 * Uses raw syscall return convention, so returns 0 for success and
 * non-zero (usually -NACL_ABI_EINVAL) for failure.
 *
 * d is a POSIX-interface descriptor
 *
 * flags may only contain one of NACL_ABI_O_RDONLY, NACL_ABI_O_WRONLY,
 * or NACL_ABI_O_RDWR, and must be the NACL_ABI_* versions of the
 * actual mode that d was opened with.  NACL_ABI_O_CREAT/APPEND are
 * permitted, but ignored, so it is safe to pass the same flags value
 * used in NaClHostDescOpen and pass it to NaClHostDescPosixDup.
 *
 * Underlying host-OS functions: dup / _dup; mode is what posix_d was
 * opened with
 */
extern int NaClHostDescPosixDup(struct NaClHostDesc *d,
                                int                 posix_d,
                                int                 flags);

/*
 * Essentially the same as NaClHostDescPosixDup, but without the dup
 * -- takes ownership of the descriptor rather than making a dup.
 */
extern int NaClHostDescPosixTake(struct NaClHostDesc  *d,
                                 int                  posix_d,
                                 int                  flags);


/*
 * Allocates a NaClHostDesc and invokes NaClHostDescPosixTake on it.
 * Aborts process if no memory.
 */
extern struct NaClHostDesc *NaClHostDescPosixMake(int posix_d,
                                                  int flags);
/*
 * Read data from an opened file into a memory buffer.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: read / _read
 */
extern ssize_t NaClHostDescRead(struct NaClHostDesc *d,
                                void                *buf,
                                size_t              len);


/*
 * Write data from a memory buffer into an opened file.
 *
 * buf is not validated.
 *
 * Underlying host-OS functions: write / _write
 */
extern ssize_t NaClHostDescWrite(struct NaClHostDesc  *d,
                                 void const           *buf,
                                 size_t               count);

extern nacl_off64_t NaClHostDescSeek(struct NaClHostDesc *d,
                                     nacl_off64_t        offset,
                                     int                 whence);

/*
 * TODO(bsy): Need to enumerate which request is supported and the
 * size of the argument, as well as whether the arg is input or
 * output.
 */
extern int NaClHostDescIoctl(struct NaClHostDesc  *d,
                             int                  request,
                             void                 *arg);

/*
 * Fstat.
 */
extern int NaClHostDescFstat(struct NaClHostDesc  *d,
                             nacl_host_stat_t     *nasp);

/*
 * Dtor for the NaClHostFile object. Close the file.
 *
 * Underlying host-OS functions:  close(2) / _close
 */
extern int NaClHostDescClose(struct NaClHostDesc  *d);

extern int NaClHostDescStat(char const        *host_os_pathname,
                            nacl_host_stat_t  *nasp);

/*
 * Maps NACI_ABI_ versions of the mmap prot argument to host ABI versions
 * of the bit values
 */

extern int NaClProtMap(int abi_prot);

EXTERN_C_END

#endif  /* defined __native_client__ */

#endif  /* NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_NACL_HOST_DESC_H__ */
