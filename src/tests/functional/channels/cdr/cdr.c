/*
 * cdr channel test
 * test plan:
 * 1. test an empty cdr channel (seek/read/append)
 *    1 try to read. EOF expected (0 bytes read, no error)
 *    2 try to write permitted number of bytes. success expected
 *    3 rewind and try to read written number of bytes. success expected
 *    4 rewind and try to write permitted number of bytes. success expected
 *    5 ensure that data were appended after the 1stly written data
 *    6 repeat from (2) until the limit reached
 *    7 ensure that after the write limit reached it is impossible to write
 *    8 reach the read limit
 * 2. test not empty cdr channel (seek/invalid read/invalid write)
 *    1 try to read negative number of bytes. check error code
 *    2 try to read from negative offset. check error code
 *    3 try to read from the offset exceed the channel size. check error code
 *    4 try to read to NULL buffer. check error code
 *    5 try to write to NULL buffer. check error code
 *    6 try to read 0 bytes. success expected
 * 3. test not empty cdr channel (seek/read/write)
 *    1 try to read. success expected
 *    .. same as (1.)
 */
#include "include/api_tools.h"

#undef STDLOG
#define STDLOG STDOUT
#define STDCDR_EMPTY "/dev/cdr_empty"
#define STDCDR_STUBBED "/dev/cdr_stubbed"

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
int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();

  /* test an empty cdr channel */
  ERRCOUNT = 0;
  initial_put(STDCDR_EMPTY);
  MSG_OK(STDCDR_EMPTY, "01234");
  MSG_OK(STDCDR_EMPTY, "567");
  MSG_OK(STDCDR_EMPTY, "89abcdefg");
  MSG_OK(STDCDR_EMPTY, "zyx");
  MSG_OK(STDCDR_EMPTY, "this is the end");
  MSG_ERR(STDCDR_EMPTY, "this write should fail");
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");

  /* test stubbed cdr channel */
  ERRCOUNT = 0;
  eofpos = zvm_bulk->channels[zhandle(STDCDR_STUBBED)].size;
  ZPRINTF(STDLOG, "%s size = %lld\n",
      zvm_bulk->channels[zhandle(STDCDR_STUBBED)].name, eofpos);
  MSG_OK(STDCDR_STUBBED, "01234");
  MSG_OK(STDCDR_STUBBED, "567");
  MSG_OK(STDCDR_STUBBED, "89abcdefg");
  MSG_OK(STDCDR_STUBBED, "zyx");
  MSG_OK(STDCDR_STUBBED, "this is the end");
  MSG_ERR(STDCDR_STUBBED, "this write should fail");

  /* exit with code */
  zput(STDLOG, ERRCOUNT ? "TEST FAILED\n\n" : "TEST SUCCEED\n\n");
  zvm_exit(0);
  return 0;
}
