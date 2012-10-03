/*
 * this sample is a simple demonstration of zerovm.
 * the "hello world" program as usual.
 */
#include "api/zvm.h"
#include <stdio.h>
#include <string.h>

#define ZVM_STDIN "/dev/stdin"
#define ZVM_STDOUT "/dev/stdout"
#define ZVM_STDERR "/dev/stderr"

static struct UserManifest *zvm_bulk = NULL;

/* get handle by the channel alias. return -1 if failed */
static inline int get_handle(const char *alias)
{
  int i;

  if(alias == NULL) return ERR_CODE;
  if(*alias == '\0') return ERR_CODE;

  for(i = 0; i < zvm_bulk->channels_count; ++i)
    if(strcmp(zvm_bulk->channels[i].name, alias) == 0)
      return i;

  return ERR_CODE;
}

/* print the message to channel. return the number of written bytes */
static inline int print(const char *alias, const char *msg)
{
  return zvm_pwrite(get_handle(alias), msg, strlen(msg), 0);
}

int main(int argc, char **argv)
{
  zvm_bulk = zvm_init();

  /* write to zvm provided stdout */
  print(ZVM_STDOUT, "hello, world!\n");

  /* write to zvm provided stderr */
  print(ZVM_STDERR, "hello, world!\n");

  /* exit with code */
  zvm_exit(0);

  /* not reached */
  return 0;
}
