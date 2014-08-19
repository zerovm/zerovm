/*
 * cdr channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */
#include "include/zvmlib.h"
#include "include/ztest.h"

#define STDCDR_EMPTY "/dev/cdr_empty"
#define STDCDR_STUBBED "/dev/cdr_stubbed"
#define STDCDR_GOAT "/dev/cdr_scapegoat"

static int64_t eofpos = 0;
static char data_start = 0;

/* put unique bytes to cdr channel. preparing channel for put_and_test() */
static void initial_put(const char *alias)
{
  WRITE(alias, "\0", 1);
  ++eofpos;
}

/*
 * write the data and check if the data written to the proper position
 * return 0 if position as expected, otherwise -1
 */
static int put_and_test(const char *alias, const char *buf, int size)
{
  int code;
  char test_buf[BIG_ENOUGH];

  /* check arguments for sanity */
  if(size < 0 || buf == NULL || alias == NULL) return -1;
  
  /* put the data and update the channel size */
  code = WRITE(alias, buf, size);
  if(code != size) return code;
  eofpos += code;

  /* read from the start channel position (0) */
  code = PREAD(alias, test_buf, size, 0);
  ZTEST(code == size);
  ZTEST(MEMCMP(buf, test_buf, size) != 0);

  /* read from the current channel position (eofpos) */
  code = PREAD(alias, test_buf, size, eofpos);
  ZTEST(code == 0);

  /* read from the proper alias position (eofpos - size) */
  code = PREAD(alias, test_buf, size, eofpos - size);
  ZTEST(code == size);
  ZTEST(MEMCMP(buf, test_buf, size) == 0);

  return 0;
}

#define MSG_OK(alias, buf) ZTEST(put_and_test((alias), buf, sizeof(buf) - 1) == 0)
#define MSG_ERR(alias, buf) ZTEST(put_and_test((alias), buf, sizeof(buf) - 1) != 0)
int main(int argc, char **argv)
{
  int overall_errors = 0;
  char buf[BIG_ENOUGH];

  /* test an empty cdr channel */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST AN EMPTY CDR\n");
  initial_put(STDCDR_EMPTY);
  MSG_OK(STDCDR_EMPTY, "01234");
  MSG_OK(STDCDR_EMPTY, "567");
  MSG_OK(STDCDR_EMPTY, "89abcdefg");
  MSG_OK(STDCDR_EMPTY, "zyx");
  MSG_OK(STDCDR_EMPTY, "this is the end");
  MSG_ERR(STDCDR_EMPTY, "this write should fail");
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test stubbed cdr channel */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST STUBBED CDR\n");
  eofpos = MANIFEST->channels[OPEN(STDCDR_STUBBED)].size;
  FPRINTF(STDERR, "%s size = %ld\n",
      MANIFEST->channels[OPEN(STDCDR_STUBBED)].name, eofpos);
  MSG_OK(STDCDR_STUBBED, "01234");
  MSG_OK(STDCDR_STUBBED, "567");
  MSG_OK(STDCDR_STUBBED, "89abcdefg");
  MSG_OK(STDCDR_STUBBED, "zyx");
  MSG_OK(STDCDR_STUBBED, "this is the end");
  MSG_ERR(STDCDR_STUBBED, "this write should fail");
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST (IN)VALID CDR WRITE CASES\n");
  ZTEST(PWRITE(STDCDR_GOAT, buf, 0, 0) == 0); /* accessing 0 bytes is always ok */
  ZTEST(PWRITE(STDCDR_GOAT, buf, -1, -1) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDCDR_GOAT, buf, -1, 0) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDCDR_GOAT, buf, -1, 1) < 0); /* invalid size, offset ignored = fail */
  ZTEST(PWRITE(STDCDR_GOAT, buf, 0, -1) < 0); /* proper size, invalid offset = fail */
  ZTEST(PWRITE(STDCDR_GOAT, buf, 1, -1) < 0); /* proper size, invalid offset = fail */
  ZTEST(PWRITE(STDCDR_GOAT, buf, 0, MANIFEST->channels[OPEN(STDCDR_GOAT)].limits[PutSizeLimit] + 1) == 0);
  ZTEST(PWRITE(STDCDR_GOAT, buf, 1, MANIFEST->channels[OPEN(STDCDR_GOAT)].limits[PutSizeLimit] + 1) == 1);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid read cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST (IN)VALID CDR READ CASES\n");
  ZTEST(PREAD(STDCDR_GOAT, buf, 0, 0) == 0); /* accessing 0 bytes is always ok */
  ZTEST(PREAD(STDCDR_GOAT, buf, -1, -1) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDCDR_GOAT, buf, -1, 0) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDCDR_GOAT, buf, -1, 1) < 0); /* invalid size, invalid offset = fail */
  ZTEST(PREAD(STDCDR_GOAT, buf, 0, -1) < 0); /* proper size, invalid offset = fail */
  ZTEST(PREAD(STDCDR_GOAT, buf, 1, -1) < 0); /* valid size, invalid offset = fail */
  ZTEST(PREAD(STDCDR_GOAT, buf, 0, MANIFEST->channels[OPEN(STDCDR_GOAT)].limits[PutSizeLimit] + 1) == 0);
  ZTEST(PREAD(STDCDR_GOAT, buf, 1, MANIFEST->channels[OPEN(STDCDR_GOAT)].limits[PutSizeLimit] + 1) == 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test NULL buffer cases */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST NULL BUFFER CASES\n");
  ZTEST(PWRITE(STDCDR_GOAT, NULL, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, NULL, 0, 0) == 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x1, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0xffff, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x10000, -1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x100000000LL - 0x1000001 - 0x10000, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x100000000LL, 1, 0) < 0);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x100000000LL - 0x1000000, 0x1000001, 0) < 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER VALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x10000, 1, 0) == 1);
  ZTEST(PWRITE(STDCDR_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size - 1, 1, 0) == 1);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(PWRITE(STDCDR_GOAT, (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pread */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(PREAD(STDCDR_GOAT, (char*)main, 1, 0) < 0);
  ZTEST(PREAD(STDCDR_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size, 1, 0) < 0);
  ZTEST(PREAD(STDCDR_GOAT, (void*)0x100000000LL - 0x1000001, 1, 0) < 0);
  ZTEST(PREAD(STDCDR_GOAT, (void*)0x100000000LL, 1, 0) < 0);
  ZTEST(PREAD(STDCDR_GOAT, (void*)0x100000000LL - 0x1000000, 0x1000001, 0) < 0);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pread */
  ERRCOUNT = 0;
  FPRINTF(STDERR, "TEST OTHER VALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(PREAD(STDCDR_GOAT, &data_start, 1, 0) == 1);
  ZTEST(PREAD(STDCDR_GOAT, MANIFEST->heap_ptr + MANIFEST->heap_size - 1, 1, 0) == 1);
  ZTEST(PREAD(STDCDR_GOAT, (void*)0x100000000LL - 0x1000000, 1, 0) < 0); /* user manifest area */
  ZTEST(PREAD(STDCDR_GOAT, (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  FPRINTF(STDERR, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* exit with code */
  if(overall_errors > 0)
    FPRINTF(STDERR, "OVERALL TEST FAILED with %d errors\n", overall_errors);
  else
    FPRINTF(STDERR, "OVERALL TEST SUCCEED\n\n");

  return overall_errors;
}
