/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*******************************************************************************
 *
 * DO NOT INCLUDE EXCEPT FROM sel_ldr.h and sel_ldr-inl.c
 *
 * THERE CANNOT BE ANY MULTIPLE INCLUSION GUARDS IN ORDER FOR
 * sel_ldr-inl.c TO WORK.
 *
 ******************************************************************************/

/*
 * Routines to translate addresses between user and "system" or
 * service runtime addresses.  the *Addr* versions will return
 * kNaClBadAddress if the user address is outside of the user address
 * space, e.g., if the input addresses for *UserToSys* is outside of
 * (1<<nap->addr_bits), and correspondingly for *SysToUser* if the
 * input system address does not correspond to a user address.
 * Generally, the *Addr* versions are used when the addresses come
 * from untrusted usre code, and kNaClBadAddress would translate to an
 * EINVAL return from a syscall.  The *Range code ensures that the
 * entire address range is in the user address space.
 *
 * Note that just because an address is within the address space, it
 * doesn't mean that it is safe to acceess the memory: the page may be
 * protected against access.
 *
 * The non-*Addr* versions abort the program rather than return an
 * error indication.
 */

/*
 * address translation routines.  after a NaClApp is started, the
 * member variables accessed by these routines are read-only, so no
 * locking is needed to use these functions, as long as the NaClApp
 * structure doesn't get destructed/deallocated.
 *
 * the first is used internally when a NULL pointer is okay, typically
 * for address manipulation.
 *
 * the next two are for syscalls to do address translation, e.g., for
 * system calls; -1 indicates an error, so the syscall can return
 * EINVAL or EFAULT or whatever is appropriate.
 *
 * the latter two interfaces are for use everywhere else in the loader
 * / service runtime and will log a fatal error and abort the process
 * when an error is detected.  (0 is not a good error indicator, since
 * 0 is a valid user address.)
 */
#include "src/service_runtime/zlog.h"

static INLINE uintptr_t NaClUserToSysAddrNullOkay(struct NaClApp  *nap,
                                                  uintptr_t       uaddr) {
  if (((uintptr_t) 1U << nap->addr_bits <= uaddr)) {
    return kNaClBadAddress;
  }
  return uaddr + nap->mem_start;
}

static INLINE uintptr_t NaClUserToSysAddr(struct NaClApp  *nap,
                                          uintptr_t       uaddr) {
  if (0 == uaddr || ((uintptr_t) 1U << nap->addr_bits) <= uaddr) {
    return kNaClBadAddress;
  }
  return uaddr + nap->mem_start;
}

/* todo(d'b): rewrite it to use in trap and setbreak */
static INLINE int NaClIsUserAddr(struct NaClApp  *nap,
                                 uintptr_t       sysaddr) {
  return nap->mem_start <= sysaddr &&
         sysaddr < nap->mem_start + ((uintptr_t) 1U << nap->addr_bits);
}

static INLINE uintptr_t NaClUserToSysAddrRange(struct NaClApp  *nap,
                                               uintptr_t       uaddr,
                                               size_t          count) {
  uintptr_t end_addr;

  if (0 == uaddr) {
    return kNaClBadAddress;
  }
  end_addr = uaddr + count;
  if (end_addr < uaddr) {
    /* unsigned wraparound */
    return kNaClBadAddress;
  }
  if (((uintptr_t) 1U << nap->addr_bits) <= end_addr) {
    return kNaClBadAddress;
  }
  return uaddr + nap->mem_start;
}

/* todo(d'b): check buffer address and size to prevent SYGSEGV */
static INLINE uintptr_t NaClUserToSys(struct NaClApp *nap, uintptr_t uaddr)
{
  ZLOGFAIL(0 == uaddr || ((uintptr_t) 1U << nap->addr_bits) <= uaddr, EFAULT,
      "NaClUserToSys: uaddr 0x%08lx, addr space %d bits\n", uaddr, nap->addr_bits);

  return uaddr + nap->mem_start;
}

static INLINE uintptr_t NaClSysToUser(struct NaClApp *nap, uintptr_t sysaddr)
{
  ZLOGFAIL(sysaddr < nap->mem_start || nap->mem_start
      + ((uintptr_t) 1U << nap->addr_bits) <= sysaddr, EFAULT,
      "NaclSysToUser: sysaddr 0x%08lx, mem_start 0x%08lx, addr space %d bits\n",
      sysaddr, nap->mem_start, nap->addr_bits);
  return sysaddr - nap->mem_start;
}

/*
 * For x86-64 sandboxing, %rsp and %rbp are system addresses already.
 */
static INLINE uintptr_t NaClUserToSysStackAddr(struct NaClApp *nap,
                                               uintptr_t      stackaddr) {
  UNREFERENCED_PARAMETER(nap);
  return stackaddr;
}

static INLINE uintptr_t NaClSysToUserStackAddr(struct NaClApp *nap,
                                               uintptr_t      stackaddr) {
  UNREFERENCED_PARAMETER(nap);
  return stackaddr;
}

static INLINE uintptr_t NaClEndOfStaticText(struct NaClApp *nap) {
  return nap->static_text_end;
}

static INLINE uintptr_t NaClSandboxCodeAddr(struct NaClApp *nap,
                                            uintptr_t addr) {
  return (((addr & ~(((uintptr_t) nap->bundle_size) - 1))
           & ((((uintptr_t) 1) << 32) - 1))
          + nap->mem_start);
}
