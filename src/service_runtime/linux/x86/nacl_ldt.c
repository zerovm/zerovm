/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl local descriptor table manipulation support.
 */

#include <asm/ldt.h>
#include <stdio.h>

#include "src/platform/nacl_sync_checked.h"
#include "src/service_runtime/arch/x86/nacl_ldt_x86.h"


// ###
///*
// * struct LdtEntry is a structure that is laid out exactly as the segment
// * descriptors described in the Intel reference manuals.  This is needed
// * because Mac and Windows use this representation in the methods used to
// * set and get entries in the local descriptor table (LDT), but use different
// * names for the members.  Linux uses a different representation to set, but
// * also reads entries back in this format.  It needs to be laid out packed,
// * bitwise, little endian.
// */
//struct LdtEntry {
//  uint16_t limit_00to15;
//  uint16_t base_00to15;
//
//  unsigned int base_16to23 : 8;
//
//  unsigned int type : 5;
//  unsigned int descriptor_privilege : 2;
//  unsigned int present : 1;
//
//  unsigned int limit_16to19 : 4;
//  unsigned int available : 1;
//  unsigned int code_64_bit : 1;
//  unsigned int op_size_32 : 1;
//  unsigned int granularity : 1;
//
//  unsigned int base_24to31 : 8;
//};

// ###
///*
// * The module initializer and finalizer set up a mutex used to guard LDT
// * manipulation.
// */
//static struct NaClMutex nacl_ldt_mutex;

// ###
//int NaClLdtInitPlatformSpecific() {
//  return NaClMutexCtor(&nacl_ldt_mutex);
//}

// ###
//void NaClLdtFiniPlatformSpecific() {
//  NaClMutexDtor(&nacl_ldt_mutex);
//}

// ###
///*
// * Find a free selector.  Always invoked while holding nacl_ldt_mutex.
// */
//static int NaClFindUnusedEntryNumber() {
//  int size = sizeof(struct LdtEntry) * LDT_ENTRIES;
//  struct LdtEntry *entries = malloc(size);
//  int i;
//
//  int retval = modify_ldt(0, entries, size);
//
//  if (-1 != retval) {
//    retval = -1;  /* In case we don't find any free entry */
//    for (i = 0; i < LDT_ENTRIES; ++i) {
//    if (!entries[i].present) {
//      retval = i;
//      break;
//      }
//    }
//  }
//
//  free(entries);
//  return retval;
//}

// ###
///*
// * Find and allocate an available selector, inserting an LDT entry with the
// * appropriate permissions.
// */
//uint16_t NaClLdtAllocateSelector(int32_t entry_number,
//                                 int size_is_in_pages,
//                                 NaClLdtDescriptorType type,
//                                 int read_exec_only,
//                                 void* base_addr,
//                                 uint32_t size_minus_one) {
//  struct user_desc ud;
//  int retval;
//
//  NaClXMutexLock(&nacl_ldt_mutex);
//
//  if (-1 == entry_number) {
//    /* -1 means caller did not specify -- allocate */
//    entry_number = NaClFindUnusedEntryNumber();
//
//    if (-1 == entry_number) {
//      /*
//       * No free entries were available.
//       */
//      goto alloc_error;
//    }
//  }
//  ud.entry_number = entry_number;
//
//  switch (type) {
//    case NACL_LDT_DESCRIPTOR_DATA:
//      ud.contents = MODIFY_LDT_CONTENTS_DATA;
//      break;
//    case NACL_LDT_DESCRIPTOR_CODE:
//      ud.contents = MODIFY_LDT_CONTENTS_CODE;
//      break;
//    default:
//      goto alloc_error;
//  }
//  ud.read_exec_only = read_exec_only;
//  ud.seg_32bit = 1;
//  ud.seg_not_present = 0;
//  ud.useable = 1;
//
//  if (size_is_in_pages && ((unsigned long) base_addr & 0xfff)) {
//    /*
//     * The base address needs to be page aligned.
//     */
//    goto alloc_error;
//  };
//  ud.base_addr = (unsigned long) base_addr;
//
//  if (size_minus_one > 0xfffff) {
//    /*
//     * If size is in pages, no more than 2**20 pages can be protected.
//     * If size is in bytes, no more than 2**20 bytes can be protected.
//     */
//    goto alloc_error;
//  }
//  ud.limit = size_minus_one;
//  ud.limit_in_pages = size_is_in_pages;
//
//  /*
//   * Install the LDT entry.
//   */
//  retval = modify_ldt(1, &ud, sizeof ud);
//  if (-1 == retval) {
//    goto alloc_error;
//  }
//
//  /*
//   * Return an LDT selector with a requested privilege level of 3.
//   */
//  NaClXMutexUnlock(&nacl_ldt_mutex);
//  return (ud.entry_number << 3) | 0x7;
//
//  /*
//   * All error returns go through this epilog.
//   */
// alloc_error:
//  NaClXMutexUnlock(&nacl_ldt_mutex);
//  return 0;
//}
