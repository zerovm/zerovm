/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define SEQRO "/dev/seqro"

/* TODO: replace it by the right include */
#define EINVAL 22

const char* target = "It is the read only target for the read";

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];

  /* correct requests */
  FPRINTF(STDERR, "TEST SEQUENTIAL READ ONLY CHANNEL\n");
  FPRINTF(STDERR, "channel size = %ld\n", MANIFEST->channels[OPEN(SEQRO)].size);
  ZTEST(MANIFEST->channels[OPEN(SEQRO)].size == 0);
  ZTEST(PREAD(SEQRO, buf, 0, 0) == 0);
  ZTEST(PREAD(SEQRO, buf, 0, 1) == 0);
  ZTEST(PREAD(SEQRO, buf, 1, 0) == 1);
  ZTEST(PREAD(SEQRO, buf, 1, MANIFEST->channels[OPEN(SEQRO)].size - 1) == 1);
  ZTEST(PREAD(SEQRO, buf, 0, -1) == 0);

  /* incorrect requests: NULL buffer */
  ZTEST(PREAD(SEQRO, NULL, 0, 0) < 0);
  ZTEST(PREAD(SEQRO, NULL, 0, 1) < 0);
  ZTEST(PREAD(SEQRO, NULL, 1, 0) < 0);
  ZTEST(PREAD(SEQRO, NULL, 1, MANIFEST->channels[OPEN(SEQRO)].size - 1) < 0);
  ZTEST(PREAD(SEQRO, NULL, 0, -1) < 0);

  /* incorrect requests: read only buffer */
  ZTEST(PREAD(SEQRO, target, 0, 0) == -EINVAL);
  ZTEST(PREAD(SEQRO, target, 0, 1) == -EINVAL);
  ZTEST(PREAD(SEQRO, target, 1, 0) == -EINVAL);
  ZTEST(PREAD(SEQRO, target, 1,
              MANIFEST->channels[OPEN(SEQRO)].size - 1) == -EINVAL);
  ZTEST(PREAD(SEQRO, target, 0, -1) == -EINVAL);

  /* incorrect requests: read only memory: MANIFEST */
  ZTEST(PREAD(SEQRO, MANIFEST->channels[OPEN(SEQRO)].name, 0, 0) == -EINVAL);
  ZTEST(PREAD(SEQRO, MANIFEST->channels[OPEN(SEQRO)].name, 0, 1) == -EINVAL);
  ZTEST(PREAD(SEQRO, MANIFEST->channels[OPEN(SEQRO)].name, 1, 0) == -EINVAL);

  /* incorrect requests: size */
  ZTEST(PREAD(SEQRO, buf, -1, 0) < 0);

  /* incorrect requests: offset */
  ZTEST(PREAD(SEQRO, buf, 0, -1) == 0);
  ZTEST(PREAD(SEQRO, buf, 1, -1) == 1);
  ZTEST(PREAD(SEQRO, buf, 1, MANIFEST->channels[OPEN(SEQRO)].size) == 1);

  /* correct requests: writing of 0 bytes attempt */
  ZTEST(PWRITE(SEQRO, buf, 0, 0) == 0);

  /* incorrect requests: write attempt */
  ZTEST(PWRITE(SEQRO, buf, 1, 0) < 0);
  ZTEST(PWRITE(SEQRO, buf, 1, MANIFEST->channels[OPEN(SEQRO)].size) < 0);
  ZTEST(PWRITE(SEQRO, buf, 1, MANIFEST->channels[OPEN(SEQRO)].size - 1) < 0);

  /* incorrect requests: exhausted */
  ZTEST(PREAD(SEQRO, buf, 30, 0) == 28);
  ZTEST(PREAD(SEQRO, buf, 10, 0) < 0);

  /* count errors and exit with it */
  ZREPORT;
  return 0;
}
