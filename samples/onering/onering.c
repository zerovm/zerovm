/*
 * test for "onering" syscall (artificial syscall)
 * here i don't use api and call "onering" directly (api i will test later and in different program)
 * update 2011-12-04: new design. manifest must be requested from nexe
 * update 2011-12-08: new design. onering called directly via syscall table (not using glibc)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "/home/dazo/sandbox/api/zvm_manifest.h"

  /*
   * WARNING!
   * nacl changes pointer in the 2nd place of "request"
   * (probably at the 1st place too)
   */

/* pointer to trampoline function */
int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of tramp record */ *
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
int32_t zvm_write(int desc, char *buffer, int32_t size, int64_t offset)
{
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  return _trap(request);
}

/* printf some policy fields */
void ShowPolicy (struct SetupList *policy)
{
  enum ChannelType ch;
  fprintf(stderr, "------------------------------------ policy begin --\n");

  /* show input fields */
  for(ch = InputChannel; ch < CHANNELS_COUNT; ++ch)
  {
    /* guts */
    fprintf(stderr, "CHANNEL NUMBER: %u\n", ch);
    fprintf(stderr, "self_size = %u\n", policy->channels[ch].self_size);
    fprintf(stderr, "handle = %d\n", policy->channels[ch].handle);
    fprintf(stderr, "name = %s\n", (char*)(uintptr_t)policy->channels[ch].name);
    fprintf(stderr, "type = %u\n", policy->channels[ch].type);
    fprintf(stderr, "mounted = %d\n", policy->channels[ch].mounted);
    fprintf(stderr, "fsize = %lld\n", policy->channels[ch].fsize);
    fprintf(stderr, "buffer = %X\n", policy->channels[ch].buffer);
    fprintf(stderr, "bsize = %d\n", policy->channels[ch].bsize);

    /* limits */
    fprintf(stderr, "max_size = %d\n", policy->channels[ch].max_size);
    fprintf(stderr, "max_gets = %d\n", policy->channels[ch].max_gets);
    fprintf(stderr, "max_puts = %d\n", policy->channels[ch].max_puts);
    fprintf(stderr, "max_get_size = %lld\n", policy->channels[ch].max_get_size);
    fprintf(stderr, "max_put_size = %lld\n", policy->channels[ch].max_put_size);

    /* counters */
    fprintf(stderr, "cnt_gets = %d\n", policy->channels[ch].cnt_gets);
    fprintf(stderr, "cnt_puts = %d\n", policy->channels[ch].cnt_puts);
    fprintf(stderr, "cnt_get_size = %lld\n", policy->channels[ch].cnt_get_size);
    fprintf(stderr, "cnt_put_size = %lld\n", policy->channels[ch].cnt_put_size);
  }

  fprintf(stderr, "------------------------------------ guts --\n");
  fprintf(stderr, "self_size = %u\n", policy->self_size);

  /* show system counters */
  fprintf(stderr, "cnt_cpu = %d\n", policy->cnt_cpu);
  fprintf(stderr, "cnt_mem = %d\n", policy->cnt_mem);
  fprintf(stderr, "cnt_setup_calls = %d\n", policy->cnt_setup_calls);
  fprintf(stderr, "cnt_syscalls = %d\n", policy->cnt_syscalls);

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
  char *buffer;
  //struct SetupList hint;
  struct SetupList *hint = malloc(sizeof(struct SetupList));

  /* request policy 1st time. construct call parameters */
  retcode = zvm_setup(hint);
  fprintf(stderr, "retcode = %d\n", retcode);
  ShowPolicy(hint);

  /* request policy 2nd time with hint */
  hint->max_mem += 1; /* try to increase amount of available memory */
  retcode = zvm_setup(hint);
  fprintf(stderr, "retcode = %d\n", retcode);
  ShowPolicy(hint);

  /* read from preloaded input */
  int chunk_size = 100;
  int position = 0;
  if((buffer = (char*) malloc(chunk_size)) != NULL)
  {
    do {
      retcode = zvm_pread(InputChannel, buffer, chunk_size, position);
      if(retcode > 0)
      {
        position += chunk_size;
        buffer[retcode] = '\0';
        fprintf(stderr, "%s", buffer);
      }
    } while(retcode > 0);
  }
  fprintf(stderr, "\nzvm_pread retcode = %d\n", retcode);

  /* write to preloaded output */
  retcode = zvm_write(OutputChannel, buffer, chunk_size, 0);
  fprintf(stderr, "zvm_write retcode = %d\n", retcode);

  /* exit with non zero to control report engine */
  return 13;
}
