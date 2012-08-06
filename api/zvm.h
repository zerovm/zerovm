/*
 * ZeroVM API. contains the ZeroVM API function prototypes, data structures,
 * macro definitions, types and ZeroVM error codes
 *
 *  Created on: Dec 2, 2011
 *      Author: d'b
 */
#ifndef API_ZVM_MANIFEST_H__
#define API_ZVM_MANIFEST_H__ 1

#include <stdint.h>
#include "zvm_errors.h"

/*
 * can be returned by zvm_read for the pure sequential
 * channels and tested with zvm_errno()
 */
#define ZVM_EOF -0x2012

/* channels available for user. very 1st channel must be InputChannel */
enum AccessType {
  SGetSPut, /* sequential read, sequential write */
  RGetSPut, /* random read, sequential write */
  SGetRPut, /* sequential read, random write */
  RGetRPut /* random read, random write */
};

/* limits/counters for i/o */
enum IOLimits {
  GetsLimit,
  GetSizeLimit,
  PutsLimit,
  PutSizeLimit,
  IOLimitsCount
};

/*
 * all the trap() syscalls resides here
 * note: the try to make user exploit function names instead of numbers
 */
enum TrapCalls {
  TrapRead = 17770431,
  TrapWrite,
  TrapSyscallback,
  TrapChannels,
  TrapChannelName,
  TrapAttributes,
  TrapSyscallsCount,
  TrapSyscallsLimit,
  TrapMemSize,
  TrapHeapPtr,
  TrapExit
};

/*
 * the file abstraction over a zerovm channel. user work with
 * zerovm channels like with regular files. meantime zerovm channels
 * could be files or network streams or anything else.
 *
 * - "optional" in comments means that fields is not necessary set
 * - "secured" field contain synthetic information
 */
struct ZVMChannel
{
  int64_t limits[IOLimitsCount];
  int64_t size; /* channel size. optional */
  enum AccessType type; /* type of access sequential/random */
#ifdef USER_SIDE
  int32_t pad2;
#endif
  char *name; /* file name (int32_t). secured */
};

/*
 * synthetic structure to substitute the older one
 * note: zerovm doesn't use it
 */
struct UserManifest
{
  /* system */
  void *heap_ptr;
  uint32_t mem_size;

  /* channels */
  int32_t channels_count;
  struct ZVMChannel *channels;

  /* limits, counters */
  int64_t syscalls_limit;

  /* user custom attributes (environment) */
  char **envp;
};

/*
 * "One Ring" wrappers and logger. disabled for trusted code
 */
#ifdef USER_SIDE

/* must be called before api usage. return 0 if successful */
struct UserManifest *zvm_init();

/* wrapper for zerovm "TrapRead" */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset);

/* wrapper for zerovm "TrapWrite" */
int32_t zvm_pwrite(int desc, const char *buffer, int32_t size, int64_t offset);

/*
 * close the channel. make a sence only for a channels with
 * sequential write. uses special form of TrapWrite call
 * return 0 if successful or -1 when error
 */
int32_t zvm_close(int desc);

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code);

/* return user heap starting address */
void* zvm_heap_ptr();

/*
 * return user memory size. note that this is not the heap
 * size, but whole memory available for user
 */
uint32_t zvm_mem_size();

/* set syscallback address. return active syscallback */
int32_t zvm_syscallback(intptr_t addr);

/*
 * if channel->name in given channel is NULL returns channel name
 * length, otherwise copy channel name to provided pointer
 */
int32_t zvm_channel_name(struct ZVMChannel *channel, int ch);

/*
 * called with NULL return channels number, otherwise copy
 * channels information into provided space
 */
int32_t zvm_channels(struct ZVMChannel *channels);

/* get zerovm error number. standard error codes were used */
int32_t zvm_errno();

/* return syscalls limit */
int32_t zvm_syscalls_limit();

#endif /* USER_SIDE */

#endif /* API_ZVM_MANIFEST_H__ */
