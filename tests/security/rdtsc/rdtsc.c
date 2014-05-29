/*
 * rdtsc test
 */
#include "api/zvm.h"

void _start()
{
  asm("rdtsc");
  z_exit(0);
}
