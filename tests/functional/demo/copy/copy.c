/*
 * copy /dev/stdin to /dev/stdout
 */
#include "include/zvmlib.h"

void _start()
{
  char buf[BIG_ENOUGH];

  for(;;)
  {
    int i = z_pread(0, buf, BIG_ENOUGH, 0);
    if(i > 0)
      i = z_pwrite(1, buf, i, 0);
    else
      break;
  }

  z_exit(0);
}
