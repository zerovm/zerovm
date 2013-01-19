/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDERR
#define INPUT "/dev/stdin"
#define OUTPUT "/dev/stdout"

int main(int argc, char **argv)
{
  int i;
  char buf[BIG_ENOUGH];
  zvm_bulk = zvm_init();

  /* pass input data to ouput */
  do {
    i = zvm_pread(zhandle(INPUT), buf, BIG_ENOUGH, 0);
    zvm_pwrite(zhandle(OUTPUT), buf, i, 0);
  } while(i > 0);

  UNREFERENCED_VAR(errcount);
  return 0;
}
