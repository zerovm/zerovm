/*
 * read/write (full access) channel test. tests statistics goes to stdout channel
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define STDRW_EMPTY "/dev/empty"
#define STDRW_STUBBED "/dev/stubbed"
#define STDRW_GOAT "/dev/scapegoat"

static char data_start = 0;

int main(int argc, char **argv)
{
  int overall_errors = 0;
  char buf[BIG_ENOUGH];

  /*
   * todo: add more cases for
   * (un)successfull writes to not empty channel
   * (un)successfull reads from not empty channel
   */

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST (IN)VALID CDR WRITE CASES\n");
  ZTEST(PWRITE(STDRW_GOAT, buf, 0, 0) == 0); /* accessing 0 bytes is always ok */
  ZTEST(PWRITE(STDRW_GOAT, buf, -1, -1) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDRW_GOAT, buf, -1, 0) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDRW_GOAT, buf, -1, 1) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDRW_GOAT, buf, 0, -1) < 0); /* invalid offset */
  ZTEST(PWRITE(STDRW_GOAT, buf, 1, -1) < 0); /* size = 1, offset invalid */
  ZTEST(PWRITE(STDRW_GOAT, buf, 0, MANIFEST->channels[OPEN(STDRW_GOAT)].limits[PutSizeLimit] + 1) == 0);
  ZTEST(PWRITE(STDRW_GOAT, buf, 1, MANIFEST->channels[OPEN(STDRW_GOAT)].limits[PutSizeLimit] + 1) < 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid reead cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST (IN)VALID CDR READ CASES\n");
  ZTEST(PREAD(STDRW_GOAT, buf, 0, 0) == 0); /* accessing 0 bytes is always ok */
  ZTEST(PREAD(STDRW_GOAT, buf, -1, -1) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDRW_GOAT, buf, -1, 0) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDRW_GOAT, buf, -1, 1) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDRW_GOAT, buf, 0, -1) < 0); /* valid size, invalid offset = fail */
  ZTEST(PREAD(STDRW_GOAT, buf, 1, -1) < 0); /* valid size, invalid offset = fail */
  ZTEST(PREAD(STDRW_GOAT, buf, 0, MANIFEST->channels[OPEN(STDRW_GOAT)].limits[PutSizeLimit] + 1) == 0); /* eof */
  ZTEST(PREAD(STDRW_GOAT, buf, 1, MANIFEST->channels[OPEN(STDRW_GOAT)].limits[PutSizeLimit] + 1) < 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test NULL buffer cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST NULL BUFFER CASES\n");
  ZTEST(PWRITE(STDRW_GOAT, NULL, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, NULL, 0, 0) == 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x1, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0xffff, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x10000, -1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x100000000LL - 0x1000001 - 0x10000, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x100000000LL, 1, 0) < 0);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x100000000LL - 0x1000000, 0x1000001, 0) < 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER VALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x10000, 1, 0) == 1);
  ZTEST(PWRITE(STDRW_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size - 1, 1, 0) == 1);
  ZTEST(PREAD(STDRW_GOAT, MANIFEST->heap_ptr, MANIFEST->heap_size + 1, 0) == 1);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(PWRITE(STDRW_GOAT, (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pread */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(PREAD(STDRW_GOAT, (char*)main, 1, 0) < 0);
  ZTEST(PREAD(STDRW_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size, 1, 0) < 0);
  ZTEST(PREAD(STDRW_GOAT, (void*)0x100000000LL - 0x1000001, 1, 0) < 0);
  ZTEST(PREAD(STDRW_GOAT, (void*)0x100000000LL, 1, 0) < 0);
  ZTEST(PREAD(STDRW_GOAT, (void*)0x100000000LL - 0x1000000, 0x1000001, 0) < 0);
  ZTEST(PREAD(STDRW_GOAT, (void*)0x100000000LL - 0x1000000, 1, 0) < 0); /* user manifest area */
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pread */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER VALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(PREAD(STDRW_GOAT, &data_start, 1, 0) == 1);
  ZTEST(PREAD(STDRW_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size - 1, 1, 0) == 1);
  ZTEST(PREAD(STDRW_GOAT, (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* exit with code */
  if(overall_errors > 0)
    FPRINTF(STDERR, "OVERALL TEST FAILED with %d errors\n", overall_errors);
  else
    FPRINTF(STDERR, "OVERALL TEST SUCCEED\n\n");

  return overall_errors;
}
