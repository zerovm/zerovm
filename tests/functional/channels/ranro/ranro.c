/*
 * random read only channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define RANRO "/dev/ranro"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];

  /* correct requests */
  FPRINTF(STDERR, "TEST RANDOM READ ONLY CHANNEL\n");
  FPRINTF(STDERR, "channel size = %d\n", MANIFEST->channels[OPEN(RANRO)].size);
  ZTEST(MANIFEST->channels[OPEN(RANRO)].size != 0);
  ZTEST(PREAD(RANRO, buf, 0, 0) == 0);
  ZTEST(PREAD(RANRO, buf, 0, 1) == 0);
  ZTEST(PREAD(RANRO, buf, 1, 0) == 1);
  ZTEST(PREAD(RANRO, buf, 1, MANIFEST->channels[OPEN(RANRO)].size - 1) == 1);
  ZTEST(PREAD(RANRO, buf, 0, -1) == 0);

  /* incorrect requests: NULL buffer */
  ZTEST(PREAD(RANRO, NULL, 0, 0) < 0);
  ZTEST(PREAD(RANRO, NULL, 0, 1) < 0);
  ZTEST(PREAD(RANRO, NULL, 1, 0) < 0);
  ZTEST(PREAD(RANRO, NULL, 1, MANIFEST->channels[OPEN(RANRO)].size - 1) < 0);
  ZTEST(PREAD(RANRO, NULL, 0, -1) < 0);

  /* incorrect requests: size */
  ZTEST(PREAD(RANRO, buf, -1, 0) < 0);

  /* incorrect requests: offset */
  ZTEST(PREAD(RANRO, buf, 0, -1) == 0);
  ZTEST(PREAD(RANRO, buf, 1, -1) < 0);
  ZTEST(PREAD(RANRO, buf, 1, MANIFEST->channels[OPEN(RANRO)].size) == 0);

  /* correct requests: writing of 0 bytes attempt */
  ZTEST(PWRITE(RANRO, buf, 0, 0) == 0);

  /* incorrect requests: write attempt */
  ZTEST(PWRITE(RANRO, buf, 1, 0) < 0);
  ZTEST(PWRITE(RANRO, buf, 1, MANIFEST->channels[OPEN(RANRO)].size) < 0);
  ZTEST(PWRITE(RANRO, buf, 1, MANIFEST->channels[OPEN(RANRO)].size - 1) < 0);

  /* incorrect requests: exhausted */
  ZTEST(PREAD(RANRO, buf, 31, 0) == 30);
  ZTEST(PREAD(RANRO, buf, 10, 0) < 0);

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
