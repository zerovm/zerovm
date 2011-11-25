/*
 * Copyright 2008 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * NaCl local descriptor table (LDT) management - common for all platforms
 */

#include "native_client/src/trusted/service_runtime/arch/x86/nacl_ldt_x86.h"
/* for LDT_ENTRIES */
#include "native_client/src/trusted/service_runtime/arch/x86/sel_ldr_x86.h"

/* TODO(gregoryd): These need to come from a header file. */
extern int NaClLdtInitPlatformSpecific();
extern int NaClLdtFiniPlatformSpecific();


int NaClLdtInit() {
  if (!NaClLdtInitPlatformSpecific()) {
    return 0;
  }
  /* Allocate the last LDT entry to force LDT grow to its maximum size */
  if (!NaClLdtAllocateSelector(LDT_ENTRIES - 1, 0,
    NACL_LDT_DESCRIPTOR_DATA, 0, 0, 0)) {
      return 0;
  }
  return 1;
}


void NaClLdtFini() {
  NaClLdtFiniPlatformSpecific();
}
