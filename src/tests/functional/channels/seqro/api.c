/*
 * the zerovm api test
 */
#include <errno.h>
#include "../include/api_tools.h"

#define BUFFER_SIZE 0x10000
#define UPDATE_RESULT(code) \
  do {\
    if(zvm_errno() == code)\
    {\
      zerror("succeed");\
    }\
    else\
    {\
      char msg[0x100];\
      snprintf(msg, 0x100, "error code = %d, expected = %d\n", zvm_errno(), code);\
      zerror("failed");\
      zput(STDERR, msg);\
      ++result;\
    }\
  } while(0)

/*
 * tests to do (ALL tests including channels, environment, command line e.t.c.):
 * 1. "zvm_bulk" initialization and values (depends on manifest)
 * 2. channels i/o: zvm_read / zvm_write (for all access types)
 * 3. environment, command line
 * 4.
 */

int main(int argc, char **argv)
{
  char buf[BUFFER_SIZE];
  int result = 0;
  zvm_bulk = zvm_init();

  /* write to zvm provided stdout */
  zput(STDOUT, "hello, world!\n");
  UPDATE_RESULT(0);

  /* summon an invalid handle error on write */
  zput("invalid name", "this message will not be processed!\n");
  UPDATE_RESULT(EBADF);

  /* summon an invalid handle error on read */
  zread("invalid name", buf, 1);
  UPDATE_RESULT(EBADF);

  /* summon an invalid buffer error on read */
  zread(STDIN, NULL, 1);
  UPDATE_RESULT(EINVAL);

  /* summon an invalid argument error on read */
  zread(STDIN, buf, -1);
  UPDATE_RESULT(EFAULT);

  /* reading of 0 byte should not invoke an error */
  zread(STDIN, buf, 0);
  UPDATE_RESULT(EFAULT);

  /* show zvm bulk information */
//  char buf[100];
//  snprintf(buf, 100, "channels_count = %d\n", zvm_bulk->channels_count);
//  print(ZVM_STDERR, buf);
//  snprintf(buf, 100, "heap_ptr = 0x%X\n", (uint32_t)zvm_bulk->heap_ptr);
//  print(ZVM_STDERR, buf);
//  snprintf(buf, 100, "mem_size = %u\n", zvm_bulk->mem_size);
//  print(ZVM_STDERR, buf);

  /* exit with code */
  zput(STDERR, result ? "TEST FAILED\n" : "TEST SUCCEED\n");
  zvm_exit(0);

  /* not reached */
  return 0;
}
