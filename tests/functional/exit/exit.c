/*
 * trap exit test
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

int main()
{
  UNREFERENCED_VAR(errcount);
  fprintf(STDERR, "TEST SUCCEED\n");
  zvm_exit(0);

  fprintf(STDERR, "OVERALL TEST FAILED with 1 error\n");
  return 0;
}
