/*
 * test for "onering" syscall (artificial syscall)
 *
 * update 2011-12-04: new design. manifest must be requested from nexe
 * update 2011-12-08: new design. onering called directly via syscall table (not using glibc)
 * update 2011-12-22: "onering" renamed to "_trap" and it should be used from "zvm.h" wrappers
 */

#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <string.h>
//#include <malloc.h>
#include "api/zvm.h"

/* printf some policy fields */
void ShowPolicy (struct SetupList *policy)
{
  enum ChannelType ch;
  fprintf(stdout, "------------------------------------ policy begin --\n");

  /* show input fields */
  for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
  {
    /* guts. some of them are not allowed for user */
    fprintf(stdout, "CHANNEL NUMBER: %u\n", ch);
    fprintf(stdout, "self_size = %u\n", policy->channels[ch].self_size);
    fprintf(stdout, "type = %u\n", policy->channels[ch].type);
    fprintf(stdout, "fsize = %lld\n", policy->channels[ch].fsize);
    fprintf(stdout, "buffer = %X\n", policy->channels[ch].buffer);
    fprintf(stdout, "bsize = %d\n", policy->channels[ch].bsize);

    /* limits */
    fprintf(stdout, "max_size = %lld\n", policy->channels[ch].max_size);
    fprintf(stdout, "max_gets = %d\n", policy->channels[ch].max_gets);
    fprintf(stdout, "max_puts = %d\n", policy->channels[ch].max_puts);
    fprintf(stdout, "max_get_size = %lld\n", policy->channels[ch].max_get_size);
    fprintf(stdout, "max_put_size = %lld\n", policy->channels[ch].max_put_size);

    /* counters not allowed for user */
  }

  fprintf(stdout, "------------------------------------ guts --\n");
  fprintf(stdout, "self_size = %u\n", policy->self_size);
  fprintf(stdout, "heap_ptr = 0x%X\n", policy->heap_ptr);

  /* show system counters (not allowed for user) */

  /* show system limits */
  fprintf(stdout, "max_cpu = %d\n", policy->max_cpu);
  fprintf(stdout, "max_mem = %d\n", policy->max_mem);
  fprintf(stdout, "max_setup_calls = %d\n", policy->max_setup_calls);
  fprintf(stdout, "max_syscalls = %d\n", policy->max_syscalls);

  /* custom attributes */
  fprintf(stdout, "content_type = %s\n", policy->content_type);
  fprintf(stdout, "timestamp = %s\n", policy->timestamp);
  fprintf(stdout, "user_etag = %s\n", policy->user_etag);
  fprintf(stdout, "x_object_meta_tag = %s\n", policy->x_object_meta_tag);

  fprintf(stdout, "------------------------------------ policy end --\n");
  fflush(NULL);
}

//int main(int argc, char **argv)
int main()
{
  int retcode;

  /* print initial values of user policy */
  ShowPolicy(&setup);
  printf("\n\n\n");

  /* request policy 2nd time with hint */
  ++setup.max_cpu; /* try to increase amount of available cpu time */
  retcode = zvm_setup(&setup);
  ShowPolicy(&setup);
  fprintf(stdout, "retcode = %d\n", retcode);

  /* exit with non zero to check report engine */
  return 13;
}
