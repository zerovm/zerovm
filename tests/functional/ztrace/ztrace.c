/*
 * test for zerovm trace engine
 * TODO: under construction
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

int main(int argc, char **argv)
{
  ZTEST(1);
  /*
   * how to test (main concept):
   * - invoke syscalls one by one (1st in line would be zvm_fork)
   * - read trace data from channel
   * - compare the data read from channel with control data
   * note: the test depends on zvm api, each time it changes this test will fail
   */

  /* count errors and exit with it */
  ZREPORT;
  return 0; /* prevent warning */
}
