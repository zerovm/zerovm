/*
 * zvm_manifest.c
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#define USER_SIDE
#include "zvm.h"
#undef USER_SIDE

static char *log; /* pointer to log buffer available space */
static int32_t log_size; /* available space in log */

/*
 * log message. 0 - if success. 1 - if log is full or has no space to
 * store the whole message (part of the message will be stored anyway)
 *
 * how to avoid static log counter and pointer?
 */
int LogMessage(char *msg)
{
  int ret_code = 0;
  int32_t msg_len;

  /*
   * check if log is not set, or full, or doesn't have enough space to
   * store whole message. check if message is not empty
   */
  if(!msg || *msg) return ERR_CODE;
  if(!log) return ERR_CODE;
  if(log_size < 1) return ERR_CODE;
  msg_len =  strlen(msg);
  if(log_size < msg_len) ret_code = ERR_CODE;

  /* append message to log buffer */
  strncat(log, msg, log_size);
  log += msg_len;
  log_size -= msg_len;

  return ret_code;
}

/* pointer to trampoline function */
int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    3 /* onering syscall number */;

/*
 * wrapper for zerovm "TrapUserSetup"
 */
int32_t zvm_setup(struct SetupList *hint)
{
  uint64_t request[] = {TrapUserSetup, 0, (uint32_t)hint};
  return _trap(request);
}

/*
 * wrapper for zerovm "TrapRead"
 */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapRead, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/*
 * wrapper for zerovm "TrapWrite"
 */
int32_t zvm_pwrite(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}
