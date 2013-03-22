/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define CHARO "/dev/charo"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];

  /* correct requests */
  FPRINTF(STDERR, "TEST SEQUENTIAL (CHARACTER DEVICE) READ ONLY CHANNEL\n");
  FPRINTF(STDERR, "channel size = %d\n", MANIFEST->channels[OPEN(CHARO)].size);
  ZTEST(MANIFEST->channels[OPEN(CHARO)].size == 0);
  ZTEST(PREAD(CHARO, buf, 0, 0) == 0);
  ZTEST(PREAD(CHARO, buf, 0, 1) == 0);
  ZTEST(PREAD(CHARO, buf, 1, 0) == 1);
  ZTEST(PREAD(CHARO, buf, 1, MANIFEST->channels[OPEN(CHARO)].size - 1) == 1);
  ZTEST(PREAD(CHARO, buf, 0, -1) == 0);

  /* incorrect handle */
  ZTEST(zvm_pread(-1, buf, 1, 0) < 0);
  ZTEST(zvm_pread(0xffff, buf, 1, 0) < 0);
  ZTEST(zvm_pread(MANIFEST->channels_count, buf, 1, 0) < 0);

  /* incorrect requests: NULL buffer */
  ZTEST(PREAD(CHARO, NULL, 0, 0) < 0);
  ZTEST(PREAD(CHARO, NULL, 0, 1) < 0);
  ZTEST(PREAD(CHARO, NULL, 1, 0) < 0);
  ZTEST(PREAD(CHARO, NULL, 1, MANIFEST->channels[OPEN(CHARO)].size - 1) < 0);
  ZTEST(PREAD(CHARO, NULL, 0, -1) < 0);

  /* incorrect requests: size */
  ZTEST(PREAD(CHARO, buf, -1, 0) < 0);

  /* incorrect requests: offset */
  ZTEST(PREAD(CHARO, buf, 0, -1) == 0);
  ZTEST(PREAD(CHARO, buf, 1, -1) == 1);
  ZTEST(PREAD(CHARO, buf, 1, MANIFEST->channels[OPEN(CHARO)].size) == 1);

  /* correct requests: writing of 0 bytes attempt */
  ZTEST(PWRITE(CHARO, buf, 0, 0) == 0);

  /* incorrect requests: write attempt */
  ZTEST(PWRITE(CHARO, buf, 1, 0) < 0);
  ZTEST(PWRITE(CHARO, buf, 1, MANIFEST->channels[OPEN(CHARO)].size) < 0);
  ZTEST(PWRITE(CHARO, buf, 1, MANIFEST->channels[OPEN(CHARO)].size - 1) < 0);

  /* incorrect requests: exhausted */
  ZTEST(PREAD(CHARO, buf, 30, 0) == 28);
  ZTEST(PWRITE(STDOUT, buf, 28, 0) == 28);
  ZTEST(PREAD(CHARO, buf, 10, 0) < 0);

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
