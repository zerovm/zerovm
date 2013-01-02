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

#ifndef STDLOG
#define STDLOG STDERR
#endif

#ifndef ERRCOUNT
#define ERRCOUNT errcount
#endif

#define BIG_ENOUGH 0x10000
#define LOT_ENOUGH 0x100

#define UNREFERENCED_VAR(a) do { (void)a; } while(0)
#define UNREFERENCED_FUNCTION(f) do {if(0) f();} while(0)

/*
 * if condition is false increase the errors count (ERRCOUNT),
 * log result to log channel (STDLOG)
 */
#define ZTEST(cond) \
  do {\
    char _msg[BIG_ENOUGH];\
    int _size;\
    if(cond)\
      _size = snprintf(_msg, BIG_ENOUGH, "succeed on %3d: %s\n", __LINE__, #cond);\
    else\
    {\
      _size = snprintf(_msg, BIG_ENOUGH, "failed on %4d: %s\n", __LINE__, #cond);\
      ++ERRCOUNT;\
    }\
    zwrite(STDLOG, _msg, _size);\
  } while(0)

/*
 * works similar to fprintf but don't have return code
 * and cannot output more than BIG_ENOUGH bytes
 */
#define ZPRINTF(alias, ...) \
  do {\
    char _buf[BIG_ENOUGH];\
    int _size;\
    _size = snprintf(_buf, BIG_ENOUGH, __VA_ARGS__);\
    zwrite(alias, _buf, _size);\
  } while(0)

static struct UserManifest *zvm_bulk = NULL;
static int errcount = 0;

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

/*
 * put the ASCIIZ message to the channel. return the number of written bytes
 * note: work only with sequential channels
 */
static inline int zput(const char *alias, const char *msg)
{
  return zvm_pwrite(zhandle(alias), msg, strlen(msg), 0);
}

/*
 * put the data to the channel. return the number of written bytes
 * note: work only with sequential channels
 */
static inline int zwrite(const char *alias, const char *buf, int32_t size)
{
  return zvm_pwrite(zhandle(alias), buf, size, 0);
}

/*
 * get the data from the channel. return the number of read bytes
 * note: work only with sequential channels
 */
static inline int zread(const char *alias, char *buf, int32_t size)
{
  return zvm_pread(zhandle(alias), buf, size, 0);
}

/* print the last zerovm error to stderr an return the error code */
static inline int zerror(const char *prefix)
{
  char msg[BIG_ENOUGH];
  int32_t size;
  int32_t code = zvm_errno();

  size = snprintf(msg, BIG_ENOUGH, "%s: %s\n", prefix, strerror(code));
  zwrite(STDERR, msg, size);
  return code;
}
