/*
 * userlog test
 * this test require manifest with "UserLog" keyword
 *
 *  Created on: Dec 12, 2011
 *      Author: d'b
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "zvm.h"

int main()
{
  struct SetupList setup;
  char *log_ptr, *log_start;
  int32_t log_size;

  /* get log address/legth */
  zvm_setup(&setup);
  log_start = log_ptr = (char*)setup.channels[LogChannel].buffer;
  log_size = setup.channels[LogChannel].bsize;
  if(log_start == 0 || log_size == 0)
  {
    fprintf(stderr, "log is not set\n");
    return 1;
  }

  /*
   * initialize log
   * note: actually buffer is already zero-filled, but we worship defensive style
   */
  log_start[0] = '\0';
#define SETPTR(ptr) ptr += strlen(ptr)

  /* write to log */
  sprintf(SETPTR(log_ptr), "hello, log\n");
  sprintf(SETPTR(log_ptr), "log buffer start found at 0x%X\n", (int32_t)log_start);
  sprintf(SETPTR(log_ptr), "log buffer size is %d\n", log_size);

  return 0;
}
