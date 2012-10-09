/*
 * this sample is a simple demonstration of zerovm.
 * the "hello world" program as usual.
 */
#include "include/api_tools.h"

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();

  UNREFERENCED_VAR(errcount);

  /* write to zvm provided stdout */
  zput(STDOUT, "hello, world!\n");

  /* write to zvm provided stderr */
  zput(STDERR, "hello, world!\n");

  /* exit with code */
  zvm_exit(0);

  /* not reached */
  return 0;
}
