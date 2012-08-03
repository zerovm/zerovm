/*
 * ZeroVM API. contain zerovm api functions
 * notice: this is an untrusted code
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#define USER_SIDE
#include "zvm.h"

/* pointer to trap() through the trampoline */
static int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    0 /* onering syscall number */;

/* holds all information which could get via api */
static struct UserManifest setup;

/* zerovm error number */
static int32_t zvm_errno_num = 0;

/* zerovm error code accessor */
int32_t zvm_errno()
{
  return zvm_errno_num;
}

/* initialization of zerovm api */
struct UserManifest *zvm_init()
{
  int i;
  struct UserManifest *result = &setup;

  /* system */
  result->heap_ptr = zvm_heap_ptr();
  result->mem_size = zvm_mem_size();

  /* get channels count */
  result->channels_count = zvm_channels(NULL);
  result->channels = calloc(result->channels_count, sizeof(*result->channels));
  zvm_channels(result->channels);

  /* channels names */
  for(i = 0; i < result->channels_count; ++i)
  {
    int length;
    struct ZVMChannel *channel = &result->channels[i];

    channel->name = NULL;
    length = zvm_channel_name(channel, i);
    channel->name = malloc(length);

    /*
     * allocate space to hold channel name
     * todo(d'b): raise here error flag
     */
    if(channel->name == NULL) break;
    zvm_channel_name(channel, i);
  }

  /* limits, counters */
  result->syscalls_limit = zvm_syscalls_limit();

  return result;
}

/*
 * wrapper for zerovm "TrapRead"
 * affects zvm_errno. can be used to detect EOF
 */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset)
{
  int32_t code;
  uint64_t request[] = {TrapRead, 0, desc, (uint32_t)buffer, size, offset};
  code = _trap(request);

  /* eof case */
  if(code == ZVM_EOF)
  {
    zvm_errno_num = code;
    return 0;
  }

  /* error encountered */
  if(code < 0)
  {
    zvm_errno_num = -code;
    return -1;
  }
  return code;
}

/*
 * wrapper for zerovm "TrapWrite"
 * affects zvm_errno
 */
int32_t zvm_pwrite(int desc, const char *buffer, int32_t size, int64_t offset)
{
  int32_t code;
  uint64_t request[] = {TrapWrite, 0, desc, (uint32_t)buffer, size, offset};
  code = _trap(request);
  if(code < 0)
  {
    zvm_errno_num = -code;
    return -1;
  }
  return code;
}

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code)
{
  uint64_t request[] = {TrapExit, 0, code};
  return _trap(request);
}

/*
 * set syscallback address. return active syscallback
 * hint: to get syscallback w/o redefining it any invalid pointer
 * could be used, for example 1
 */
int32_t zvm_syscallback(intptr_t addr)
{
  uint64_t request[] = {TrapSyscallback, 0, addr};
  return _trap(request);
}

/* return user heap starting address */
void* zvm_heap_ptr()
{
  uint64_t request[] = {TrapHeapPtr};
  return (void*)_trap(request);
}

/*
 * return user memory size. note that this is not the heap
 * size, but whole memory available for user
 */
uint32_t zvm_mem_size()
{
  uint64_t request[] = {TrapMemSize};
  return _trap(request);
}

/*
 * if channel->name in given channel is NULL returns channel name
 * length, otherwise copy channel name to provided pointer
 * affects zvm_errno (if invalid argument given)
 */
int32_t zvm_channel_name(struct ZVMChannel *channel, int ch)
{
  int32_t code;
  uint64_t request[] = {TrapChannelName, 0, (intptr_t)channel, ch};
  code = _trap(request);
  if(code < 0)
  {
    zvm_errno_num = -code;
    return -1;
  }
  return code;
}

/*
 * called with NULL return channels number, otherwise copy
 * channels information into provided space
 */
int32_t zvm_channels(struct ZVMChannel *channels)
{
  uint64_t request[] = {TrapChannels, 0, (intptr_t)channels};
  return _trap(request);
}

/* return syscalls limit */
int32_t zvm_syscalls_limit()
{
  uint64_t request[] = {TrapSyscallsLimit};
  return _trap(request);
}

#undef USER_SIDE
