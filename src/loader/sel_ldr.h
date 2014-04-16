/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * NaCl Simple/secure ELF loader (NaCl SEL).
 *
 * notice: "NaCl" is here by historical reasons. in fact zvm only can
 * use binaries built with ZeroVM toolchain (former NaCl toolchain)
 *
 * This loader can only process ZeroVM object files as produced using
 * the ZeroVM toolchain.  Other ELF files will be rejected.
 *
 * The primary function, NaClAppLoadFile, parses an ELF file,
 * allocates memory, loads the relocatable image from the ELF file
 * into memory, and performs relocation.  NaClAppRun runs the
 * resultant program.
 */

#ifndef SEL_LDR_H_
#define SEL_LDR_H_ 1

#include <stdint.h>

/* from sel_ldr_x86.h */
#define ADDR_BITS (32)
#define NACL_HALT_OPCODE   0xf4
#define NACL_HALT_LEN      1 /* length of halt instruction */

#include "src/platform/gio.h"
#include "src/main/config.h"
#include "src/loader/sel_rt.h"
#include "src/main/manifest.h"

EXTERN_C_BEGIN

struct NaClApp {
  /* only used for ET_EXEC:  for CS restriction */
  uintptr_t                 static_text_end;  /* relative to mem_start */
  /* ro after app starts. memsz from phdr */

  /*
   * rodata_start and data_start may be 0 if these segments are not
   * present in the executable.
   */
  uintptr_t                 rodata_start;  /* initialized data, ro */
  uintptr_t                 data_start;    /* initialized data/bss, rw */
  /*
   * Various region sizes must be a multiple of NACL_MAP_PAGESIZE
   * before the NaCl app can run.  The sizes from the ELF file
   * (p_filesz field) might not be -- that would waste space for
   * padding -- and while we could use p_memsz to specify padding, but
   * we will record the virtual addresses of the start of the segments
   * and figure out the gap between the p_vaddr + p_filesz of one
   * segment and p_vaddr of the next to determine padding.
   */

  uintptr_t                 data_end;
  /* see break_addr below */

  /*
   * initial_entry_pt is the first address in untrusted code to jump
   * to. initial_entry_pt is in the user executable.
   */
  uintptr_t                 initial_entry_pt;

  uintptr_t                 break_addr; /* user addr */
  /* data_end <= break_addr is an invariant */

  struct Manifest           *manifest;
};

/* global variables */
struct ThreadContext    *nacl_user; /* user registers storage */
struct ThreadContext    *nacl_sys;  /* zerovm registers storage */
struct NaClApp              *gnap;  /* pointer to global NaClApp object */

/*
 * Initializes a NaCl application with the default parameters.
 * nap is a pointer to the NaCl object that is being filled in.
 * assumes all fields already set to zeroes
 */
void NaClAppCtor(struct NaClApp *nap);

/* free app memory and globals */
void NaClAppDtor(struct NaClApp *nap);

/*
 * Loads a NaCl ELF file into memory in preparation for running it.
 *
 * gp is a pointer to a generic I/O object and should be a GioMem with
 * a memory buffer containing the file read entirely into memory if
 * the file system might be subject to race conditions (e.g., another
 * thread / process might modify a downloaded NaCl ELF file while we
 * are loading it here).
 *
 * nap is a pointer to the NaCl object that is being filled in.  it
 * should be properly constructed via NaClAppCtor.
 *
 * note: it may be necessary to flush the icache if the memory
 * allocated for use had already made it into the icache from another
 * NaCl application instance, and the icache does not detect
 * self-modifying code / data writes and automatically invalidate the
 * cache lines.
 */
void AppLoadFile(struct Gio *gp, struct NaClApp *nap);

/* TODO(d'b): replace spaces with format options and use macro */
void PrintAppDetails(struct NaClApp *nap, int verbosity);

/*
 * set an empty user stack and other context, then pass
 * control to user code
 */
void RunSession(struct NaClApp *nap);

#include "src/loader/sel_ldr-inl.h"

void FillMemoryRegionWithHalt(void *start, size_t size);

int ThreadContextCtor(struct ThreadContext  *ntcp,
                      struct NaClApp        *nap,
                      nacl_reg_t            prog_ctr,
                      nacl_reg_t            stack_ptr);

EXTERN_C_END

#endif  /* SEL_LDR_H_ */
