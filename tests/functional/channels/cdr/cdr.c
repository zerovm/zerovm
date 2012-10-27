/*
 * cdr channel test. tests statistics goes to stdout channel.
 * returns the number of failed tests
 */

static char data_start = 0;

#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT
#define STDCDR_EMPTY "/dev/cdr_empty"
#define STDCDR_STUBBED "/dev/cdr_stubbed"
#define STDCDR_GOAT "/dev/cdr_scapegoat"
static int64_t eofpos = 0;

/* put unique bytes to cdr channel. preparing channel for put_and_test() */
static void initial_put(const char *alias)
{
  zwrite(alias, "\0", 1);
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
  code = zwrite(alias, buf, size);
  if(code != size) return code;
  eofpos += code;

  /* read from the start channel position (0) */
  code = zvm_pread(zhandle(alias), test_buf, size, 0);
  ZTEST(code == size);
  ZTEST(memcmp(buf, test_buf, size) != 0);

  /* read from the current channel position (eofpos) */
  code = zvm_pread(zhandle(alias), test_buf, size, eofpos);
  ZTEST(code == 0);

  /* read from the proper alias position (eofpos - size) */
  code = zvm_pread(zhandle(alias), test_buf, size, eofpos - size);
  ZTEST(code == size);
  ZTEST(memcmp(buf, test_buf, size) == 0);

  return 0;
}

#define MSG_OK(alias, buf) ZTEST(put_and_test((alias), buf, sizeof(buf) - 1) == 0)
#define MSG_ERR(alias, buf) ZTEST(put_and_test((alias), buf, sizeof(buf) - 1) != 0)
#define WRITEGOAT(size, offset, result) \
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), buf, size, offset) == result);
#define READGOAT(size, offset, result) \
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), buf, size, offset) == result);
int main(int argc, char **argv)
{
  int overall_errors = 0;
  char buf[BIG_ENOUGH];
  zvm_bulk = zvm_init();

  /* test an empty cdr channel */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST AN EMPTY CDR\n");
  initial_put(STDCDR_EMPTY);
  MSG_OK(STDCDR_EMPTY, "01234");
  MSG_OK(STDCDR_EMPTY, "567");
  MSG_OK(STDCDR_EMPTY, "89abcdefg");
  MSG_OK(STDCDR_EMPTY, "zyx");
  MSG_OK(STDCDR_EMPTY, "this is the end");
  MSG_ERR(STDCDR_EMPTY, "this write should fail");
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test stubbed cdr channel */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST STUBBED CDR\n");
  eofpos = zvm_bulk->channels[zhandle(STDCDR_STUBBED)].size;
  ZPRINTF(STDLOG, "%s size = %lld\n",
      zvm_bulk->channels[zhandle(STDCDR_STUBBED)].name, eofpos);
  MSG_OK(STDCDR_STUBBED, "01234");
  MSG_OK(STDCDR_STUBBED, "567");
  MSG_OK(STDCDR_STUBBED, "89abcdefg");
  MSG_OK(STDCDR_STUBBED, "zyx");
  MSG_OK(STDCDR_STUBBED, "this is the end");
  MSG_ERR(STDCDR_STUBBED, "this write should fail");
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST (IN)VALID CDR WRITE CASES\n");
  WRITEGOAT(0, 0, 0); /* accessing of 0 bytes is always ok */
  WRITEGOAT(-1, -1, -1); /* invalid size, offset ignored = fail */
  WRITEGOAT(-1, 0, -1); /* invalid size, offset ignored = fail */
  WRITEGOAT(-1, 1, -1); /* invalid size, offset ignored = fail */
  WRITEGOAT(0, -1, 0); /* accessing of 0 bytes is always ok */
  WRITEGOAT(1, -1, 1); /* size = 1, offset ignored = 1 byte written */
  WRITEGOAT(0, zvm_bulk->channels[zhandle(STDCDR_GOAT)].limits[PutSizeLimit] + 1, 0);
  WRITEGOAT(1, zvm_bulk->channels[zhandle(STDCDR_GOAT)].limits[PutSizeLimit] + 1, 1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test (in)valid write cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST (IN)VALID CDR READ CASES\n");
  READGOAT(0, 0, 0); /* accessing of 0 bytes is always ok */
  READGOAT(-1, -1, -1); /* invalid size, invalid offset = fail */
  READGOAT(-1, 0, -1); /* invalid size, valid offset = fail */
  READGOAT(-1, 1, -1); /* invalid size, valid offset = fail */
  READGOAT(0, -1, 0); /* accessing of 0 bytes is always ok */
  READGOAT(1, -1, -1); /* valid size, invalid offset = fail */
  READGOAT(0, zvm_bulk->channels[zhandle(STDCDR_GOAT)].limits[PutSizeLimit] + 1, -1);
  READGOAT(1, zvm_bulk->channels[zhandle(STDCDR_GOAT)].limits[PutSizeLimit] + 1, -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test NULL buffer cases */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST NULL BUFFER CASES\n");
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), NULL, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), NULL, 0, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x1, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0xffff, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x10000, -1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr, zvm_bulk->heap_size + 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000001, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x100000000LL, 1, 0) == -1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000000, 0x1000001, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pwrite */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER VALID BUFFER/SIZE CASES FOR PWRITE\n");
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x10000, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size - 1, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(zvm_pwrite(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1, 1, 0) == 1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other invalid buffer address/size cases for pread */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER INVALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (char*)main, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr, zvm_bulk->heap_size + 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000001, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (void*)0x100000000LL, 1, 0) == -1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000000, 0x1000001, 0) == -1);
  overall_errors += ERRCOUNT;
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test other valid buffer address/size cases for pread */
  ERRCOUNT = 0;
  ZPRINTF(STDLOG, "TEST OTHER VALID BUFFER/SIZE CASES FOR PREAD\n");
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), &data_start, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), zvm_bulk->heap_ptr + zvm_bulk->heap_size - 1, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1000000, 1, 0) == 1);
  ZTEST(zvm_pread(zhandle(STDCDR_GOAT), (void*)0x100000000LL - 0x1, 1, 0) == 1);
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
