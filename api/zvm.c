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

static char *_log; /* pointer to log buffer available space */
static int32_t _log_size; /* available space in log */

/* pointer to trap() through the trampoline */
int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    0 /* onering syscall number */;

/* setup log */
int log_set(struct UserManifest *setup)
{
  struct ChannelDesc *channel;
  if(setup == NULL) return PARAM_CODE;

  channel = &((struct ChannelDesc*)(intptr_t)setup->channels)[LogChannel];
  _log = (char*)channel->buffer;
  _log_size = channel->bsize;

  return _log == NULL ? PTR_CODE : OK_CODE;
}

/* log message */
/*
 * OBSOLETTE.
 * known issue: the message will be append if log is already exist
 * make it feature or fix?
 * todo(d'b): replace it with log_msg2() from zrt.c
 */
int log_msg(char *msg)
{
  int ret_code = 0;
  int32_t msg_len;

  /*
   * check if log is not set, or full, or doesn't have enough space to
   * store whole message. check if message is not empty
   */
  if(!_log) return PTR_CODE;
  if(_log_size < 1) return SIZE_CODE;
  if(!msg || !*msg) return PARAM_CODE;
  msg_len =  strlen(msg);
  if(_log_size < msg_len) ret_code = LARGE_CODE;

  /* append message to log buffer */
  strncat(_log, msg, _log_size);
  _log += msg_len;
  _log_size -= msg_len;

  return ret_code;
}

/* wrapper for zerovm "TrapUserSetup" */
int32_t zvm_setup(struct UserManifest *hint)
{
  uint64_t request[] = {TrapUserSetup, 0, (uint32_t)hint};
  return _trap(request);
}

/* wrapper for zerovm "TrapRead" */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapRead, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/* wrapper for zerovm "TrapWrite" */
int32_t zvm_pwrite(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code)
{
  uint64_t request[] = {TrapExit, 0, code};
  return _trap(request);
}

#undef USER_SIDE
