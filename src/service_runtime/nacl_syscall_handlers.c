/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "include/nacl_platform.h"
#include "include/nacl_macros.h"
#include "src/platform/nacl_sync_checked.h"
#include "src/desc/nacl_desc_io.h"
#include "src/desc/nacl_desc_mutex.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_syscall_handlers.h"
#include "src/service_runtime/sel_memory.h"
#include "src/service_runtime/include/bits/mman.h"
#include "src/service_runtime/include/bits/nacl_syscalls.h"
#include "src/service_runtime/include/sys/stat.h"
#include "src/service_runtime/linux/nacl_syscall_inl.h"
#include "src/manifest/trap.h"
#include "include/nacl_assert.h"

#include "src/manifest/manifest_setup.h" //### to make manifest work

struct NaClSyscallTableEntry nacl_syscall[NACL_MAX_SYSCALLS] = {{0}};
static const size_t kMaxUsableFileSize = (SIZE_T_MAX >> 1);
static INLINE size_t  size_min(size_t a, size_t b) {
  return (a < b) ? a : b;
}

static int AddrRangeContainsExecutablePages(struct NaClApp *nap, uintptr_t usraddr)
{
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
  usraddr = NaClTruncAllocPage(usraddr);
  return usraddr < nap->dynamic_text_end;
}

static void NaClCommonUtilUpdateAddrMap(struct NaClApp *nap, uintptr_t sysaddr, size_t nbytes,
    int sysprot, struct NaClDesc *backing_desc, nacl_off64_t backing_bytes,
    nacl_off64_t offset_bytes, int delete_mem)
{
  uintptr_t usraddr;
  struct NaClMemObj *nmop;

  usraddr = NaClSysToUser(nap, sysaddr);
  nmop = NULL;
  /* delete_mem -> NULL == backing_desc */
  if(NULL != backing_desc)
  {
    if(delete_mem)
    {
      NaClLog(LOG_FATAL, ("invariant of delete_mem implies backing_desc NULL"
              " violated.\n"));
    }
    nmop = NaClMemObjMake(backing_desc, backing_bytes, offset_bytes);
  }

  NaClVmmapUpdate(&nap->mem_map, usraddr >> NACL_PAGESHIFT,
                  nbytes >> NACL_PAGESHIFT, sysprot, nmop, delete_mem);
}

static int32_t NotImplementedDecoder(struct NaClApp *nap)
{
  UNREFERENCED_PARAMETER(nap);
  return -NACL_ABI_ENOSYS;
}

static void NaClAddSyscall(int num, int32_t(*fn)(struct NaClApp *))
{
  if(nacl_syscall[num].handler != &NotImplementedDecoder)
  {
    NaClLog(LOG_FATAL, "Duplicate syscall number %d\n", num);
  }
  nacl_syscall[num].handler = fn;
}

/*
 * syscalls main routines.
 * note that we only need 1 syscall trap() other syscalls are only used
 * for nexe prolog in a future all syscalls except trap can be removed.
 */

int32_t NaClSysSysbrk(struct NaClApp *nap, uintptr_t new_break)
{
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

  break_addr = nap->break_addr;

  NaClLog(3, "Entered NaClSetBreak(new_break 0x%08"NACL_PRIxPTR")\n", new_break);

  sys_new_break = NaClUserToSysAddr(nap, new_break);
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

      /*
       * use "whole chunk" memory manager if set in manifest
       * and this is "user side call"
       */
      if(nap->user_side_flag && nap->manifest != NULL &&
          nap->manifest->user_setup->max_mem)
      {
        /* ### skip real mprotect() syscall. or shell we? */
      }
      else /* proceed real syscall */
      {
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

  /*
   * This cast is safe because the incoming value (new_break) cannot
   * exceed the user address space--even though its type (uintptr_t)
   * theoretically allows larger values.
   */
  rv = (int32_t) break_addr;

  NaClLog(3, "NaClSetBreak: returning 0x%08"NACL_PRIx32"\n", rv);
  return rv;
}

int32_t NaClCommonSysMmapIntern(struct NaClApp        *nap,
                                void                  *start,
                                size_t                length,
                                int                   prot,
                                int                   flags,
                                int                   d,
                                nacl_abi_off_t        offset) {
  int                         allowed_flags;
  struct NaClDesc             *ndp = NULL;
  uintptr_t                   usraddr;
  uintptr_t                   usrpage;
  uintptr_t                   sysaddr;
  uintptr_t                   endaddr;
  uintptr_t                   map_result;
  struct NaClMemObj           *nmop = NULL;
  struct nacl_abi_stat        stbuf;
  size_t                      alloc_rounded_length;
  nacl_off64_t                file_size;
  nacl_off64_t                file_bytes;
  nacl_off64_t                host_rounded_file_bytes;
  size_t                      alloc_rounded_file_bytes;
  size_t                      start_of_inaccessible;

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
    NaClLog(1, ("NaClSysMmap: file offset 0x%08"NACL_PRIxPTR" not multiple"
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
    NaClLog(1, "mmap: rounded length to 0x%"NACL_PRIxS"\n",
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
    NaClLog(4, "NaClCommonSysMmapIntern: file_size = %ld, offset = %ld\n", file_size, offset);

    if (file_size < offset) {
      map_result = -NACL_ABI_EINVAL;
      goto cleanup;
    }

    file_bytes = file_size - offset;
    NaClLog(4, "NaClCommonSysMmapIntern: file_bytes 0x%016"NACL_PRIxNACL_OFF"\n",
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

  if(AddrRangeContainsExecutablePages(nap, usraddr))
  {
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

      NaClLog(4, ("NaClSysMmap: (*ndp->Map)(,,0x%08"NACL_PRIxPTR","
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

int32_t NaClSysMmap(struct NaClApp        *nap,
                    void                  *start,
                    size_t                length,
                    int                   prot,
                    int                   flags,
                    int                   d,
                    nacl_abi_off_t        *offp) {
  int32_t         retval;
  uintptr_t       sysaddr;
  nacl_abi_off_t  offset;

  NaClLog(3, "Entered NaClSysMmap(0x%08"NACL_PRIxPTR",0x%"NACL_PRIxS","
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

  sysaddr = NaClUserToSysAddrRange(nap, (uintptr_t) offp, sizeof offset);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(3,
            "NaClCommonSysMmap: offset in a bad untrusted memory location\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }
  offset = *(nacl_abi_off_t volatile *) sysaddr;

  NaClLog(4, " offset = 0x%08"NACL_PRIxNACL_OFF"\n", offset);
  retval = NaClCommonSysMmapIntern(nap, start, length, prot, flags, d, offset);

cleanup:
  return retval;
}

int32_t NaClSysMunmap(struct NaClApp *nap, void *start, size_t length) {
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sysaddr;
  int       holding_app_lock = 0;
  size_t    alloc_rounded_length;

  NaClLog(3, "Entered NaClSysMunmap(0x%08"NACL_PRIxPTR", "
          "0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS")\n",
          (uintptr_t) nap, (uintptr_t) start, length);

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
  sysaddr = NaClUserToSysAddrRange(nap, (uintptr_t) start, length);
  if (kNaClBadAddress == sysaddr) {
    NaClLog(4, "region not user addresses\n");
    retval = -NACL_ABI_EFAULT;
    goto cleanup;
  }

  NaClXMutexLock(&nap->mu);

  while (0 != nap->threads_launching) {
    NaClXCondVarWait(&nap->cv, &nap->mu);
  }
  nap->vm_hole_may_exist = 1;

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
  if (AddrRangeContainsExecutablePages(nap, (uintptr_t) start)) {
    NaClLog(2, "NaClSysMunmap: region contains executable pages\n");
    retval = -NACL_ABI_EINVAL;
    goto cleanup;
  }

  /*
   * Overwrite current mapping with inaccessible, anonymous
   * zero-filled pages, which should be copy-on-write and thus
   * relatively cheap.  Do not open up an address space hole.
   */
  NaClLog(4, ("NaClSysMunmap: mmap(0x%08"NACL_PRIxPTR", 0x%"NACL_PRIxS","
           " 0x%x, 0x%x, -1, 0)\n"), sysaddr, length, PROT_NONE,
          MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED);

  /*
   * use "whole chunk" memory manager if set in manifest
   * and this is "user side call"
   */
  if(nap->user_side_flag && nap->manifest != NULL &&
      nap->manifest->user_setup->max_mem)
  {
    /* ### skip real syscall. all allowed memory already allocated */
  }
  else
  {
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
  }

  NaClVmmapUpdate(&nap->mem_map,
                  (NaClSysToUser(nap, (uintptr_t) sysaddr)
                   >> NACL_PAGESHIFT),
                  length >> NACL_PAGESHIFT,
                  0,  /* prot */
                  (struct NaClMemObj *) NULL,
                  1);  /* Delete mapping */
  retval = 0;
cleanup:
  if (holding_app_lock) {
    nap->vm_hole_may_exist = 0;
    NaClXCondVarBroadcast(&nap->cv);
    NaClXMutexUnlock(&nap->mu);
  }
  return retval;
}

/* d'b: duplicate of trap() exit. remove it when trap() replace syscalls */
int32_t NaClSysExit(struct NaClApp *nap, int status)
{
  NaClLog(1, "Exit syscall handler: %d\n", status);
  nap->exit_status = status;
  nap->running = 0;
  longjmp(user_exit, status);

  /* NOTREACHED */
  return -NACL_ABI_EINVAL;
}

/* tls create */
#define CLEANUP(cond, code) if(cond) {retval = code; goto cleanup;}
int32_t NaClSysTls_Init(struct NaClApp *nap, void *thread_ptr)
{
  int32_t   retval = -NACL_ABI_EINVAL;
  uintptr_t sys_tls;

  NaClLog(3, ("Entered NaClCommonSysTls_Init(0x%08"NACL_PRIxPTR
      ", 0x%08"NACL_PRIxPTR")\n"), (uintptr_t) nap, (uintptr_t) thread_ptr);

  /* Verify that the address in the app's range and translated from
   * nacl module address to service runtime address - a nop on ARM
   */
  sys_tls = NaClUserToSysAddrRange(nap, (uintptr_t) thread_ptr, 4);
  NaClLog(4, "NaClCommonSysTls_Init: thread_ptr 0x%p, sys_tls 0x%"NACL_PRIxPTR"\n",
          thread_ptr, sys_tls);

  CLEANUP(kNaClBadAddress == sys_tls, -NACL_ABI_EFAULT);
  nap->sys_tls = sys_tls;
  retval = 0;

cleanup:
  return retval;
}
#undef CLEANUP

/* tls get */
int32_t NaClSysTls_Get(struct NaClApp *nap) {
  uint32_t user_tls;

  /* too frequently used, and syscall-number level logging suffices */
  user_tls = (int32_t) NaClSysToUser(nap, nap->sys_tls);
  return user_tls;
}

/* mutex */
int32_t NaClSysMutex_Create(struct NaClApp *nap)
{
  int32_t              retval = -NACL_ABI_EINVAL;
  struct NaClDescMutex *desc;

  NaClLog(3,
          ("Entered NaClCommonSysMutex_Create(0x%08"NACL_PRIxPTR")\n"),
          (uintptr_t) nap);

  desc = malloc(sizeof(*desc));

  if (!desc || !NaClDescMutexCtor(desc)) {
    retval = -NACL_ABI_ENOMEM;
    goto cleanup;
  }

  retval = NaClSetAvail(nap, (struct NaClDesc *)desc);
  desc = NULL;
cleanup:
  free(desc);
  NaClLog(3, ("NaClCommonSysMutex_Create(0x%08"NACL_PRIxPTR") = %d\n"),
          (uintptr_t) nap, retval);
  return retval;
}

/*
 * decoders. get call parameters from the user stack and call the syscall
 */

/* this function was automagically generated */
static int32_t NaClSysSysbrkDecoder(struct NaClApp *nap) {
  struct NaClSysSysbrkArgs {
    uint32_t new_break;
  } p = *(struct NaClSysSysbrkArgs *) nap->syscall_args;

  return NaClSysSysbrk(nap, (uintptr_t) p.new_break);
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

  /*
   * d'b: fixed bug when user can violate limits using opened file descriptor
   * from the manifest. here we override descriptor with -1 to prevent it.
   */
  return NaClSysMmap(nap, (void *) (uintptr_t) p.start, (size_t)  p.length, (int)  p.prot, (int)  p.flags, -1, (nacl_abi_off_t *) (uintptr_t) p.offp);
}

/* this function was automagically generated */
static int32_t NaClSysMunmapDecoder(struct NaClApp *nap) {
  struct NaClSysMunmapArgs {
    uint32_t start;
    uint32_t length;
  } p = *(struct NaClSysMunmapArgs *) nap->syscall_args;

  return NaClSysMunmap(nap, (void *) (uintptr_t) p.start, (size_t)  p.length);
}

/* this function was automagically generated */
static int32_t NaClSysExitDecoder(struct NaClApp *nap) {
  struct NaClSysExitArgs {
    uint32_t status;
  } p = *(struct NaClSysExitArgs *) nap->syscall_args;

  return NaClSysExit(nap, (int)  p.status);
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
static int32_t NaClSysTls_GetDecoder(struct NaClApp *nap)
{
  return NaClSysTls_Get(nap);
}

/* this function was automagically generated */
static int32_t NaClSysMutex_CreateDecoder(struct NaClApp *nap)
{
  return NaClSysMutex_Create(nap);
}

/* d'b: trap(). see documentation for the details */
static int32_t TrapDecoder(struct NaClApp *nap)
{
  struct TrapDecoderArgs
  {
    uint32_t args;
  } p = *(struct TrapDecoderArgs*) nap->syscall_args;

  return TrapHandler(nap, p.args);
}

/*
 * d'b: see documentation about "one ring" tonneling syscall
 * note: all syscalls except trap() only need for nexe prolog
 */
void NaClSyscallTableInit() {
  int i;
  for (i = 0; i < NACL_MAX_SYSCALLS; ++i) {
     nacl_syscall[i].handler = &NotImplementedDecoder;
  }

  NaClAddSyscall(Trap, &TrapDecoder); /* 0. added onering call */
  NaClAddSyscall(NACL_sys_sysbrk, &NaClSysSysbrkDecoder); /* 20 */
  NaClAddSyscall(NACL_sys_exit, &NaClSysExitDecoder); /* 30 */
  NaClAddSyscall(NACL_sys_tls_init, &NaClSysTls_InitDecoder); /* 82 */
  NaClAddSyscall(NACL_sys_tls_get, &NaClSysTls_GetDecoder); /* 84 */
  NaClAddSyscall(NACL_sys_mutex_create, &NaClSysMutex_CreateDecoder); /* 70 */
  /* 21. syscall is not used by current nexe prolog, but can be invoked by large mallocs */
  NaClAddSyscall(NACL_sys_mmap, &NaClSysMmapDecoder);
  NaClAddSyscall(NACL_sys_munmap, &NaClSysMunmapDecoder); /* same here */
}
