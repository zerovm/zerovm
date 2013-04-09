/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/zvmlib.h"

void _start()
{
  int i;

  /* pass input data to ouput */
  for(;;)
  {
    char buf[BIG_ENOUGH];

    i = zvm_pread(0, buf, BIG_ENOUGH, 0);
    if(i <= 0) break;

    i = zvm_pwrite(1, buf, i, 0);
    if(i <= 0) break;
  };

  zvm_exit(i);
}
