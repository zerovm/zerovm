/*
 * test for "onering" syscall (artificial syscall)
 *
 * update 2011-12-04: new design. manifest must be requested from nexe
 * update 2011-12-08: new design. onering called directly via syscall table (not using glibc)
 * update 2011-12-22: "onering" renamed to "_trap" and it should be used from "zvm.h" wrappers
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

#include "zvm.h"

/* printf some policy fields */
void ShowPolicy (struct SetupList *policy)
{
  enum ChannelType ch;
  fprintf(stderr, "------------------------------------ policy begin --\n");

  /* show input fields */
  for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
  {
    /* guts. some of them are not allowed for user */
    fprintf(stderr, "CHANNEL NUMBER: %u\n", ch);
    fprintf(stderr, "self_size = %u\n", policy->channels[ch].self_size);
    fprintf(stderr, "type = %u\n", policy->channels[ch].type);
    fprintf(stderr, "fsize = %lld\n", policy->channels[ch].fsize);
    fprintf(stderr, "buffer = %X\n", policy->channels[ch].buffer);
    fprintf(stderr, "bsize = %d\n", policy->channels[ch].bsize);

    /* limits */
    fprintf(stderr, "max_size = %lld\n", policy->channels[ch].max_size);
    fprintf(stderr, "max_gets = %d\n", policy->channels[ch].max_gets);
    fprintf(stderr, "max_puts = %d\n", policy->channels[ch].max_puts);
    fprintf(stderr, "max_get_size = %lld\n", policy->channels[ch].max_get_size);
    fprintf(stderr, "max_put_size = %lld\n", policy->channels[ch].max_put_size);

    /* counters not allowed for user */
  }

  fprintf(stderr, "------------------------------------ guts --\n");
  fprintf(stderr, "self_size = %u\n", policy->self_size);
  fprintf(stderr, "heap_ptr = 0x%X\n", policy->heap_ptr);

  /* show system counters (not allowed for user) */

  /* show system limits */
  fprintf(stderr, "max_cpu = %d\n", policy->max_cpu);
  fprintf(stderr, "max_mem = %d\n", policy->max_mem);
  fprintf(stderr, "max_setup_calls = %d\n", policy->max_setup_calls);
  fprintf(stderr, "max_syscalls = %d\n", policy->max_syscalls);

  /* custom attributes */
  fprintf(stderr, "content_type = %s\n", policy->content_type);
  fprintf(stderr, "timestamp = %s\n", policy->timestamp);
  fprintf(stderr, "user_etag = %s\n", policy->user_etag);
  fprintf(stderr, "x_object_meta_tag = %s\n", policy->x_object_meta_tag);

  fprintf(stderr, "------------------------------------ policy end --\n");
}

int main(int argc, char **argv)
{
  int retcode;
  struct SetupList setup;
  struct SetupList *hint = &setup;

  /* request policy 1st time. construct call parameters */
  retcode = zvm_setup(hint);
  fprintf(stderr, "retcode = %d\n", retcode);
  ShowPolicy(hint);

  /* request policy 2nd time with hint */
  hint->max_mem += 1; /* try to increase amount of available memory */
  retcode = zvm_setup(hint);
  fprintf(stderr, "retcode = %d\n", retcode);
  ShowPolicy(hint);

  /* exit with non zero to control report engine */
  return 13;
}
