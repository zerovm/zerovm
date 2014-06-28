/*
 * this sample only useful to generate assembly code and
 * see the call invocation
 */
#include "api/zvm.h"

static int dummy(int a, int b, int c)
{
  return a + b + c;
}

void _start()
{
  int a = 0x11111111;
  int b = 0x22222222;
  int c = 0x33333333;

  z_exit(dummy(a, b, c));
}
