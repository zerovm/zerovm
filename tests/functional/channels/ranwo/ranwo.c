/*
 * random write only channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define RANWO "/dev/ranwo"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];

  /* correct requests */
  FPRINTF(STDERR, "TEST RANDOM WRITE ONLY CHANNEL\n");
  FPRINTF(STDERR, "channel size = %ld\n", MANIFEST->channels[OPEN(RANWO)].size);
  ZTEST(MANIFEST->channels[OPEN(RANWO)].size == 0);
  ZTEST(PWRITE(RANWO, buf, 0, 0) == 0);

  ZTEST(PWRITE(RANWO, buf, 0, 0) == 0);
  ZTEST(PWRITE(RANWO, buf, 0, 1) == 0);
  ZTEST(PWRITE(RANWO, buf, 1, 0) == 1);
  ZTEST(PWRITE(RANWO, buf, 1,
      MANIFEST->channels[OPEN(RANWO)].limits[PutSizeLimit] - 1) == 1);
  ZTEST(PWRITE(RANWO, buf, 0, -1) == 0);

  /* incorrect requests: NULL buffer */
  ZTEST(PWRITE(RANWO, NULL, 0, 0) < 0);
  ZTEST(PWRITE(RANWO, NULL, 0, 1) < 0);
  ZTEST(PWRITE(RANWO, NULL, 1, 0) < 0);
  ZTEST(PWRITE(RANWO, NULL, 1,
      MANIFEST->channels[OPEN(RANWO)].limits[PutSizeLimit] - 1) < 0);
  ZTEST(PWRITE(RANWO, NULL, 0, -1) < 0);

  /* incorrect requests: size */
  ZTEST(PWRITE(RANWO, buf, -1, 0) < 0);

  /* incorrect requests: offset */
  ZTEST(PWRITE(RANWO, buf, 0, -1) == 0);
  ZTEST(PWRITE(RANWO, buf, 1, -1) < 0);
  ZTEST(PWRITE(RANWO, buf, 1, MANIFEST->channels[OPEN(RANWO)].limits[PutSizeLimit]) < 0);

  /* correct requests: reading of 0 bytes attempt */
  ZTEST(PREAD(RANWO, buf, 0, 0) == 0);

  /* incorrect requests: read attempt */
  ZTEST(PREAD(RANWO, buf, 1, 0) < 0);
  ZTEST(PREAD(RANWO, buf, 1, MANIFEST->channels[OPEN(RANWO)].limits[PutSizeLimit]) < 0);

  /* incorrect requests: exhausted */
  ZTEST(PWRITE(RANWO, buf, 31, 0) == 30);
  ZTEST(PWRITE(RANWO, buf, 10, 0) < 0);

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
