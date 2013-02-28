/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/zvmlib.h"

int main(int argc, char **argv)
{
  int i;

  /* pass input data to ouput */
  for(;;)
  {
    char buf[BIG_ENOUGH];

    i = READ(STDIN, buf, BIG_ENOUGH);
    if(i <= 0) break;

    WRITE(STDOUT, buf, i);
    if(i <= 0) break;
  };

  return i;
}
