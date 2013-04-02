/*
 * trap exit test
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

int main()
{
  UNREFERENCED_VAR(errcount);
  fprintf(STDERR, "TEST SUCCEED\n");
  zvm_exit(1234);

  /* to prevent falling into endless loop provoke SIGSEGV */
  fprintf(STDERR, "OVERALL TEST FAILED with 1 error\n");
  asm("hlt");
  return 0;
}
