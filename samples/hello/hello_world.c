/*
 * "hello, world" example.
 * user log is used istead of stdout/stderr
 *
 * todo: use fprintf(stderr, ...) with syscallback
 */
#include <stdio.h>
#include "api/zvm.h"

int main(void)
{
  int retcode = ERR_CODE;

  /* set up the log */
  struct SetupList setup;
  retcode = zvm_setup(&setup);
  if(retcode) return retcode;
  retcode = log_set(&setup);
  if(retcode) return retcode;

  /* log "hello, world" message */
  return log_msg("hello, world!\n");
}
