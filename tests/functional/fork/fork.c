/*
 * zvm trap fork test
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

int main()
{
  UNREFERENCED_VAR(errcount);
  fprintf(STDERR, "before fork()\n");
  zvm_fork();

  /* this part will be run in forked session */
  printf("stdout: after fork()\n");
  fprintf(STDERR, "after fork()\n");
  return 0;
}
