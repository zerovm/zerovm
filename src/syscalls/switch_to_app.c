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

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "src/main/tools.h"
#include "src/main/zlog.h"
#include "src/syscalls/switch_to_app.h"

/* AMD instruction sets */
#ifdef __XOP__
# include <x86intrin.h> /* AMD XOP (GNU) */
#elif defined (__SSE4A__) /* AMD SSE4A */
# include <ammintrin.h>
#endif

/* CPUID. "r" should be int[4], "func" = eax */
#define CPUID(r, func) \
    asm("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3]) : "a"(func), "c"(0))

/*
 * find supported instruction set, return value:
 * 0           = 80386 instruction set
 * 1  or above = SSE (XMM) (not testing for O.S. support)
 * 2  or above = SSE2
 * 3  or above = SSE3
 * 4  or above = SSSE3 (Supplementary SSE3)
 * 5  or above = SSE4.1
 * 6  or above = SSE4.2
 * 7  or above = AVX
 * 8  or above = AVX2
 */
static int CPUTest(void)
{
  uint32_t a, d;
  int r[4] = {0};

  CPUID(r, 0);
  if(r[0] == 0) return 0;
  CPUID(r, 1);

#define TEST_CPU(reg, bit, result) \
  do {if((r[(reg)] & (1 << (bit))) == 0) return(result);} while(0)

  TEST_CPU(3, 0, 0);
  TEST_CPU(3, 23, 0);
  TEST_CPU(3, 15, 0);
  TEST_CPU(3, 24, 0);
  TEST_CPU(3, 25, 0);
  TEST_CPU(3, 26, 1);
  TEST_CPU(2, 0, 2);
  TEST_CPU(2, 9, 3);
  TEST_CPU(2, 19, 4);
  TEST_CPU(2, 23, 5);
  TEST_CPU(2, 20, 5);
  TEST_CPU(2, 27, 6);

  asm("xgetbv" : "=a"(a),"=d"(d) : "c"(0));
  if(((a | (((uint64_t)d) << 32)) & 6) != 6) return 6;

  TEST_CPU(2, 28, 6);
  CPUID(r, 7);
  TEST_CPU(1, 5, 7);

  return 8;
#undef TEST_CPU
}

void InitSwitchToApp()
{
  int cpu = CPUTest();
  char *name[] = {"no SSE", "SSE", "SSE2", "SSE3", "Supplementary SSE3",
                  "SSE4.1", "SSE4.2", "AVX", "AVX2 or better"};

  assert((unsigned)cpu < ARRAY_SIZE_SAFE(name));

  ZLOGS(LOG_DEBUG, "%s cpu detected", name[cpu]);
  ZLOGFAIL(cpu == 0, EFAULT, "zerovm needs at least SSE CPU");
  ZLOGIF(cpu > 7, "zerovm running on CPU with partial support. UNSAFE!");
  ContextSwitch = cpu < 7 ? SwitchSSE : SwitchAVX;
}
