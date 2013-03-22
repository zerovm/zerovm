/*
 * random write only channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define SEQWO "/dev/seqwo"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];

  /* correct requests */
  FPRINTF(STDERR, "TEST SEQUENTIAL WRITE ONLY CHANNEL\n");
  FPRINTF(STDERR, "channel size = %lld\n", MANIFEST->channels[OPEN(SEQWO)].size);
  ZTEST(MANIFEST->channels[OPEN(SEQWO)].size == 0);
  ZTEST(PWRITE(SEQWO, buf, 0, 0) == 0);
  ZTEST(PWRITE(SEQWO, buf, 0, 1) == 0);
  ZTEST(PWRITE(SEQWO, buf, 1, 0) == 1);
  ZTEST(PWRITE(SEQWO, buf, 1,
      MANIFEST->channels[OPEN(SEQWO)].limits[PutSizeLimit] - 1) == 1);
  ZTEST(PWRITE(SEQWO, buf, 0, -1) == 0);

  /* incorrect requests: NULL buffer */
  ZTEST(PWRITE(SEQWO, NULL, 0, 0) < 0);
  ZTEST(PWRITE(SEQWO, NULL, 0, 1) < 0);
  ZTEST(PWRITE(SEQWO, NULL, 1, 0) < 0);
  ZTEST(PWRITE(SEQWO, NULL, 1,
      MANIFEST->channels[OPEN(SEQWO)].limits[PutSizeLimit] - 1) < 0);
  ZTEST(PWRITE(SEQWO, NULL, 0, -1) < 0);

  /* incorrect requests: size */
  ZTEST(PWRITE(SEQWO, buf, -1, 0) < 0);

  /* incorrect requests: offset */
  ZTEST(PWRITE(SEQWO, buf, 0, -1) == 0);
  ZTEST(PWRITE(SEQWO, buf, 1, -1) == 1);
  ZTEST(PWRITE(SEQWO, buf, 1,
      MANIFEST->channels[OPEN(SEQWO)].limits[PutSizeLimit]) == 1);

  /* correct requests: reading of 0 bytes attempt */
  ZTEST(PREAD(SEQWO, buf, 0, 0) == 0);

  /* incorrect requests: read attempt */
  ZTEST(PREAD(SEQWO, buf, 1, 0) < 0);
  ZTEST(PREAD(SEQWO, buf, 1,
      MANIFEST->channels[OPEN(SEQWO)].limits[PutSizeLimit]) < 0);

  /* incorrect requests: exhausted */
  ZTEST(PWRITE(SEQWO, buf, 30, 0) == 28);
  ZTEST(PWRITE(SEQWO, buf, 10, 0) < 0);

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
