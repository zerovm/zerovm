/*
 * zvm_manifest.c
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#define USER_SIDE
#include "api/zvm_manifest.h"
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
  if(log_size < msg_len) ret_code = ERR_CODE;

  /* append message to log buffer */
  msg_len =  strlen(msg);
  strncat(log, msg, log_size);
  log += msg_len;
  log_size -= msg_len;

  return ret_code;
}

/* positional read file */
int _trap_pread(enum ChannelType desc, void *buffer, size_t size, off_t offset)
{
  int64_t in[] = { TrapRead, desc, buffer, size, offset };
  int64_t out[1]; /* amount of read bytes or errno */
  return nanosleep(in, out) ? ERR_CODE : *out;
  // ### set errno like: errno = *out
}

/* positional write file */
int _trap_pwrite(enum ChannelType desc, void *buffer, size_t size, off_t offset)
{
  int64_t in[] = { TrapWrite, desc, buffer, size, offset };
  int64_t out[1]; /* amount of written bytes or errno */
  return nanosleep(in, out) ? ERR_CODE : *out;
  // ### set errno like: errno = *out
}

/* get (update) user policy */
int _trap_setup(struct UserSetup *hint)
{
  int64_t in[] = { TrapUserSetup, hint };
  if(hint) return nanosleep(in, NULL);
  return ERR_CODE;
}
