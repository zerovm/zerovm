/*
 * "hello world" through zerovm api with no data segment
 */
#include "api/zvm.h"

#define HELLO 0x77202c6f6c6c6548ULL
#define WORLD 0x0a21646c726fULL

void _start()
{
  uint64_t hi[] = {HELLO, WORLD};
  z_pwrite(1, hi, 14, 0);
  z_exit(0);
}
