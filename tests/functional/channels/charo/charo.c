/*
 * sequential read only channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT
#define CHARO "/dev/charo"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];
  zvm_bulk = zvm_init();

  /* correct requests */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST SEQUENTIAL (CHARACTER DEVICE) READ ONLY CHANNEL\n");
  ZPRINTF(STDLOG, "channel size = %lld\n", zvm_bulk->channels[zhandle(CHARO)].size);
  ZTEST(zvm_bulk->channels[zhandle(CHARO)].size == 0);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 0, 0) == 0);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 0, 1) == 0);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 1, zvm_bulk->channels[zhandle(CHARO)].size - 1) == 1);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 0, -1) == 0);

  /* incorrect handle */
  ZTEST(zvm_pread(-1, buf, 1, 0) == -1);
  ZTEST(zvm_pread(0xffff, buf, 1, 0) == -1);
  ZTEST(zvm_pread(zvm_bulk->channels_count, buf, 1, 0) == -1);

  /* incorrect requests: NULL buffer */
  ZTEST(zvm_pread(zhandle(CHARO), NULL, 0, 0) == -1);
  ZTEST(zvm_pread(zhandle(CHARO), NULL, 0, 1) == -1);
  ZTEST(zvm_pread(zhandle(CHARO), NULL, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(CHARO), NULL, 1, zvm_bulk->channels[zhandle(CHARO)].size - 1) == -1);
  ZTEST(zvm_pread(zhandle(CHARO), NULL, 0, -1) == -1);

  /* incorrect requests: size */
  ZTEST(zvm_pread(zhandle(CHARO), buf, -1, 0) == -1);

  /* incorrect requests: offset */
  ZTEST(zvm_pread(zhandle(CHARO), buf, 0, -1) == 0);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 1, -1) == 1);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 1, zvm_bulk->channels[zhandle(CHARO)].size) == 1);

  /* correct requests: writing of 0 bytes attempt */
  ZTEST(zvm_pwrite(zhandle(CHARO), buf, 0, 0) == 0);

  /* incorrect requests: write attempt */
  ZTEST(zvm_pwrite(zhandle(CHARO), buf, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(CHARO), buf, 1, zvm_bulk->channels[zhandle(CHARO)].size) == -1);
  ZTEST(zvm_pwrite(zhandle(CHARO), buf, 1, zvm_bulk->channels[zhandle(CHARO)].size - 1) == -1);

  /* incorrect requests: exhausted */
  ZTEST(zvm_pread(zhandle(CHARO), buf, 30, 0) == 28);
  ZTEST(zvm_pwrite(zhandle(STDERR), buf, 28, 0) == 28);
  ZTEST(zvm_pread(zhandle(CHARO), buf, 10, 0) == -1);

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
    ZPRINTF(STDLOG, "TEST FAILED with %d errors\n", ERRCOUNT);
  else
    zput(STDLOG, "TEST SUCCEED\n\n");

  zvm_exit(ERRCOUNT);
  return 0;
}
