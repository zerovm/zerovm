/*
 * random write only channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */
#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT
#define SEQWO "/dev/seqwo"

int main(int argc, char **argv)
{
  char buf[BIG_ENOUGH];
  zvm_bulk = zvm_init();

  /* correct requests */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST SEQUENTIAL WRITE ONLY CHANNEL\n");
  ZPRINTF(STDLOG, "channel size = %lld\n", zvm_bulk->channels[zhandle(SEQWO)].size);
  ZTEST(zvm_bulk->channels[zhandle(SEQWO)].size == 0);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 0, 0) == 0);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 0, 1) == 0);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 1,
      zvm_bulk->channels[zhandle(SEQWO)].limits[PutSizeLimit] - 1) == 1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 0, -1) == 0);

  /* incorrect requests: NULL buffer */
  ZTEST(zvm_pwrite(zhandle(SEQWO), NULL, 0, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), NULL, 0, 1) == -1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), NULL, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), NULL, 1,
      zvm_bulk->channels[zhandle(SEQWO)].limits[PutSizeLimit] - 1) == -1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), NULL, 0, -1) == -1);

  /* incorrect requests: size */
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, -1, 0) == -1);

  /* incorrect requests: offset */
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 0, -1) == 0);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 1, -1) == 1);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 1,
      zvm_bulk->channels[zhandle(SEQWO)].limits[PutSizeLimit]) == 1);

  /* incorrect requests: read attempt */
  ZTEST(zvm_pread(zhandle(SEQWO), buf, 0, 0) == -1);
  ZTEST(zvm_pread(zhandle(SEQWO), buf, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(SEQWO), buf, 1,
      zvm_bulk->channels[zhandle(SEQWO)].limits[PutSizeLimit]) == -1);

  /* incorrect requests: exhausted */
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 30, 0) == 28);
  ZTEST(zvm_pwrite(zhandle(SEQWO), buf, 10, 0) == -1);

  /* count errors and exit with it */
  if(ERRCOUNT > 0)
    ZPRINTF(STDLOG, "TEST FAILED with %d errors\n", ERRCOUNT);
  else
    zput(STDLOG, "TEST SUCCEED\n\n");

  zvm_exit(ERRCOUNT);
  return 0;
}
