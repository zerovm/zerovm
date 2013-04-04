/*
 * rdtsc test
 */
#include "api/zvm.h"

void _start()
{
  asm("rdtsc");
  zvm_exit(0);
}
