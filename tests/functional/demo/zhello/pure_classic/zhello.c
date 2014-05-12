/*
 * "hello world" through zerovm api
 */
#include "api/zvm.h"

void _start()
{
  zvm_pwrite(1, "hello, world!\n", 14, 0);
  zvm_exit(0);
}
