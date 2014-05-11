/*
 * hello world through zerovm api (main is omitted, _start used instead)
 */
#include "api/zvm.h"

/* constants */
#define stdout 1
#define stderr 2

/* functions */
#define fput(handle, msg) zvm_pwrite(handle, msg, sizeof msg, 0)
#define exit(c) zvm_exit(c)

void _start()
{
  fput(stdout, "\033[1mhello from dev/stdout!\033[0m\n");
  fput(stderr, "\033[36mhello from dev/stderr!\033[0m\n");

  exit(0);
}
