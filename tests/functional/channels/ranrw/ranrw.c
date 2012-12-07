/*
 * read/write (full access) channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */

static char data_start = 0;

#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT
#define STDRW_EMPTY "/dev/empty"
#define STDRW_STUBBED "/dev/stubbed"
#define STDRW_GOAT "/dev/scapegoat"

#define WRITEGOAT(size, offset, result) \
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), buf, size, offset) == result);
#define READGOAT(size, offset, result) \
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), buf, size, offset) == result);
int main(int argc, char **argv)
{
  int overall_errors = 0;
  char buf[BIG_ENOUGH];
  zvm_bulk = zvm_init();

  /* (un)successfull writes to an empty channel */
  /* (un)successfull writes to not empty channel */
  /* unsuccessfull reads from an empty channel */
  /* (un)successfull reads from not empty channel */

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST (IN)VALID RW WRITE CASES\n");
  WRITEGOAT(0, 0, 0); /* accessing of 0 bytes is always ok */
  WRITEGOAT(-1, -1, -1); /* invalid size, invalid offset = fail */
  WRITEGOAT(-1, 0, -1); /* invalid size, valid offset = fail */
  WRITEGOAT(-1, 1, -1); /* invalid size, valid offset = fail */
  WRITEGOAT(0, -1, 0); /* accessing of 0 bytes is always ok */
  WRITEGOAT(1, -1, -1); /* size = 1, offset ignored = 1 byte written */
  WRITEGOAT(0, zvm_bulk->channels[zhandle(STDRW_GOAT)].limits[PutSizeLimit] + 1, 0);
  WRITEGOAT(1, zvm_bulk->channels[zhandle(STDRW_GOAT)].limits[PutSizeLimit] + 1, -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST (IN)VALID RW READ CASES\n");
  READGOAT(0, 0, 0); /* accessing of 0 bytes is always ok */
  READGOAT(-1, -1, -1); /* invalid size, invalid offset = fail */
  READGOAT(-1, 0, -1); /* invalid size, valid offset = fail */
  READGOAT(-1, 1, -1); /* invalid size, valid offset = fail */
  READGOAT(0, -1, 0); /* accessing of 0 bytes is always ok */
  READGOAT(1, -1, -1); /* valid size, invalid offset = fail */
  READGOAT(0, zvm_bulk->channels[zhandle(STDRW_GOAT)].limits[PutSizeLimit] + 1, -1);
  READGOAT(1, zvm_bulk->channels[zhandle(STDRW_GOAT)].limits[PutSizeLimit] + 1, -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test NULL buffer cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST NULL BUFFER CASES\n");
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), NULL, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), NULL, 0, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x1, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0xffff, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x10000, -1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr, zvm_bulk->heap_size + 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000001, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x100000000LL, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000000, 0x1000001, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER VALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x10000, 1, 0) == 1); // ???
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size - 1, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pread */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (char*)main, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr, zvm_bulk->heap_size + 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000001, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (void*)0x100000000LL, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000000, 0x1000001, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pread */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER VALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), &data_start, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size - 1, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDRW_GOAT), (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* exit with code */
  if(overall_errors > 0)
    ZPRINTF(STDLOG, "OVERALL TEST FAILED with %d errors\n", overall_errors);
  else
    zput(STDLOG, "OVERALL TEST SUCCEED\n\n");

  zvm_exit(overall_errors);
  return 0;
}
