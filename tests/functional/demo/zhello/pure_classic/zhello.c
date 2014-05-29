/*
 * "hello world" through zerovm api
 */
#include "api/zvm.h"

void _start()
{
  z_pwrite(1, "hello, world!\n", 14, 0);
  z_exit(0);
}
