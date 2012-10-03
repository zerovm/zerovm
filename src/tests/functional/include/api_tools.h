/*
 * this header contain helper functions, definitions and variables
 * to work with zerovm api
 */

/* to ensure that zvm api is set on */
#ifndef USER_SIDE
#define USER_SIDE
#endif

#include <stdio.h>
#include <string.h>
#include "api/zvm.h"

#define STDIN "/dev/stdin"
#define STDOUT "/dev/stdout"
#define STDERR "/dev/stderr"

static struct UserManifest *zvm_bulk = NULL;

/* get handle by the channel alias. return -1 if failed */
static inline int zhandle(const char *alias)
{
  int i;

  if(alias == NULL) return ERR_CODE;
  if(*alias == '\0') return ERR_CODE;

  for(i = 0; i < zvm_bulk->channels_count; ++i)
    if(strcmp(zvm_bulk->channels[i].name, alias) == 0)
      return i;

  return ERR_CODE;
}

/* put the message to the channel. return the number of written bytes */
static inline int zput(const char *alias, const char *msg)
{
  return zvm_pwrite(zhandle(alias), msg, strlen(msg), 0);
}

/* put the data to the channel. return the number of written bytes */
static inline int zwrite(const char *alias, const char *buf, int32_t size)
{
  return zvm_pwrite(zhandle(alias), buf, size, 0);
}

/* get the data from the channel. return the number of read bytes */
static inline int zread(const char *alias, char *buf, int32_t size)
{
  return zvm_pread(zhandle(alias), buf, size, 0);
}

/* print the last zerovm error to stderr an return the error code */
#define MSG_SIZE 0x10000
static inline int zerror(const char *prefix)
{
  char msg[MSG_SIZE];
  int32_t size;
  int32_t code = zvm_errno();

  size = snprintf(msg, MSG_SIZE, "%s: %s\n", prefix, strerror(code));
  zwrite(STDERR, msg, size);
  return code;
}
#undef MSG_SIZE
