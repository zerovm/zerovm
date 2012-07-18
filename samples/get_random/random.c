/*
 * demonstration of zerovm hole which violates determinism
 * each run this program gets different values from trampoline
 *
 * update: zerovm fixed. this example can be removed or included
 *         as part of the security test
 */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "api/zrt.h"

#define TRAMPOLINE_START (intptr_t)0x10000
#define SYSCALLS_NUMBER 0x10000/0x20

int main()
{
  int32_t i;
  int32_t *tramp_ptr = (int32_t*)TRAMPOLINE_START;

  for(i = 0; i < SYSCALLS_NUMBER; i += 8)
  {
    fprintf(stdout, "%4d: %08X %08X %08X %08X %08X %08X %08X %08X\n", i, 
            tramp_ptr[i + 0],
            tramp_ptr[i + 1],
            tramp_ptr[i + 2],
            tramp_ptr[i + 3],
            tramp_ptr[i + 4],
            tramp_ptr[i + 5],
            tramp_ptr[i + 6],
            tramp_ptr[i + 7]);
  }

  return 0;
}
