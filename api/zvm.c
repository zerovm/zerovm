/*
 * ZeroVM API
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */

#include <string.h>
#include <stdio.h>

#define USER_SIDE
#include "zvm.h"

/* user manifest holder */
static struct UserManifest mft;

/* pointer to trap() through the trampoline */
int32_t (*_trap)(uint64_t *in) = (int32_t (*)(uint64_t*))
    0x10000 /* start of trampoline */ +
    0x20 /* size of trampoline record */ *
    0 /* onering syscall number */;

/* initialization of zerovm api */
/*
 * todo(d'b): get number of channels here
 */
struct UserManifest* zvm_init()
{
  return &mft;
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

/*
 * UserManifest data accessors
 */

/* return user heap starting address */
void* zvm_heap_start()
{
  return (void*)mft.heap_ptr;
}

/*
 * return user memory size. note that this is not the heap
 * size, but whole memory available for user
 */
uint32_t zvm_memory_size()
{
  return mft.max_mem;
}

/* return content_type or NULL */
char* zvm_content_type()
{
  return *mft.content_type ? mft.content_type : NULL;
}

/* return timestamp or NULL */
char* zvm_timestamp()
{
  return *mft.timestamp ? mft.timestamp : NULL;
}

/* return x_object_meta_tag or NULL */
char* zvm_x_object_meta_tag()
{
  return *mft.x_object_meta_tag ? mft.x_object_meta_tag : NULL;
}

/* return user_etag or NULL */
char* zvm_user_etag()
{
  return *mft.user_etag ? mft.user_etag : NULL;
}

/* return syscallback address or 0 */
int32_t zvm_syscallback()
{
  return mft.syscallback;
}

/* set syscallback to the new address, return -1 if failed */
int32_t zvm_set_syscallback(int32_t syscallback)
{
  mft.syscallback = syscallback;
  return zvm_setup(&mft);
}

/* return memory buffer address or -1 if not available */
void* zvm_channel_addr(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return (void*)channel->buffer;
}

/* return memory buffer size or -1 if not available */
int32_t zvm_channel_size(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->bsize;
}

/* return name of the channel or NULL. note: secured field (does not contain real name) */
char* zvm_channel_name(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  static char *names[] = {
      "InputChannel",
      "OutputChannel",
      "LogChannel",
      "NetworkInputChannel",
      "NetworkOutputChannel"
  };

  if(ch < InputChannel && ch >= ChannelTypesCount) return NULL;

  /* if name is set by zerovm return it, otherwise return nominal one */
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->name ? (char*)(intptr_t)channel->name : names[(int)ch];
  /* (int)ch cast to prevent compiler warning */
}

/* return handle of the channel. note: secured field */
int32_t zvm_channel_handle(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->handle;
}

/* return type of the channel */
enum ChannelType zvm_channel_type(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->type;
}

/* return mount mode of the channel. note: in a future can be secured */
enum MountMode zvm_channel_mode(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->mounted;
}

/* return channel file size. note: in a future can be secured */
int64_t zvm_channel_fsize(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->fsize;
}

/* return get size limit for the given channel */
int64_t zvm_channel_get_size_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->limits[GetSizeLimit];
}

/* return gets count limit for the given channel */
int64_t zvm_channel_get_count_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->limits[GetsLimit];
}

/* return put size limit for the given channel */
int64_t zvm_channel_put_size_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->limits[PutSizeLimit];
}

/* return puts count limit for the given channel */
int64_t zvm_channel_put_count_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->limits[PutsLimit];
}

/* return get size counter for the given channel */
int64_t zvm_channel_get_size_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->counters[GetSizeCounter];
}

/* return gets count counter for the given channel */
int64_t zvm_channel_get_count_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->counters[GetsCounter];
}

/* return put size counter for the given channel */
int64_t zvm_channel_put_size_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->counters[PutSizeCounter];
}

/* return puts count counter for the given channel */
int64_t zvm_channel_put_count_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)mft.channels)[ch];
  return channel->counters[PutsCounter];
}

#undef USER_SIDE
