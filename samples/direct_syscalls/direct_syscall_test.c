/*
 * direct invocation of nacl syscall
 * this test require manifest with "UserLog" keyword
 *
 *  Created on: Dec 20, 2012
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
  if(log_start == 0 || log_size == 0) return 1;

  /*
   * initialize log
   * note: actually buffer is already zero-filled, but we worship defensive style
   */
  log_start[0] = '\0';
#define SETPTR(ptr) ptr += strlen(ptr)

  /* log 1st message */
  sprintf(SETPTR(log_ptr), "this message must appear in the user log\n");

  /*
   * bellow we can see direct nacl syscall example.
   * this syscall (exit in our case) invokes via trampoline* (see documentation)
   * any of 56 nacl syscalls can be used directly by the shown way
   */
#define NaCl_exit(code) ((int32_t (*)(uint32_t)) (30*0x20 + 0x10000))(code)

  /* call syscall exit() (the syscall number is 30) */
  NaCl_exit(0);

  /* try to log 2nd message. but we already exited, aren't we?  */
  sprintf(SETPTR(log_ptr), "this message must NOT appear in the user log\n");

  return 2;
}
