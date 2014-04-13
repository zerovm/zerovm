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

#include "src/loader/sel_ldr.h"

void FillMemoryRegionWithHalt(void *start, size_t size)
{
  ZLOGFAIL(size % NACL_HALT_LEN, EFAULT, FAILED_MSG);
  memset(start, NACL_HALT_OPCODE, size);
}

void NaClAppCtor(struct NaClApp *nap)
{
  gnap = nap;
  nacl_user = g_malloc(sizeof *nacl_user);
  nacl_sys = g_malloc(sizeof *nacl_sys);
}

void NaClAppDtor(struct NaClApp *nap)
{
  g_free(nacl_sys);
  g_free(nacl_user);
  nacl_sys = NULL;
  nacl_user = NULL;
}

void PrintAppDetails(struct NaClApp *nap, int verbosity)
{
  ZLOGS(verbosity, "NaClAppPrintDetails((struct NaClApp*) 0x%08lx,", (uintptr_t)nap);
  ZLOGS(verbosity, "addr space size:         2**%d", ADDR_BITS);
  ZLOGS(verbosity, "stack size:              0x%08d", STACK_SIZE);
  ZLOGS(verbosity, "mem start addr:          0x%08lx", MEM_START);
  ZLOGS(verbosity, "static_text_end:         0x%08lx", nap->static_text_end);
  ZLOGS(verbosity, "end-of-text:             0x%08lx", NaClEndOfStaticText(nap));
  ZLOGS(verbosity, "rodata:                  0x%08lx", nap->rodata_start);
  ZLOGS(verbosity, "data:                    0x%08lx", nap->data_start);
  ZLOGS(verbosity, "data_end:                0x%08lx", nap->data_end);
  ZLOGS(verbosity, "break_addr:              0x%08lx", nap->break_addr);
  ZLOGS(verbosity, "ELF initial entry point: 0x%08x", nap->initial_entry_pt);
}
