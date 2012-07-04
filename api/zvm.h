/*
 * UNDER CONSTRUCTION! MAY CHANGE. MAY CONTAIN ERRORS.
 * ZeroVM manifest header. contain the structure of user side manifest.
 * nexe can access this object via arguments as usual, e.g:
 * main(int argc, char **argv). argv[0] will contain the manifest if
 * strlen(argv[0]) == 0
 * it would be handy to cast argv[0] to Manifest:
 * struct Manifest *manifest = (struct Manifest*) *argv;
 *
 * 2011-11-30
 * update: "struct Manifest" has been changed. everything given to nexe
 * now has plain text format (in a future can be changed to json), so user
 * program must parse text part of manifest to obtain given keys/values
 *
 * 2011-12-01
 * update: now it is recommended to use api functions instead of using manifest
 * directly. now user can request changes in policy, there is, however, no garantee
 * that zvm will accept user settings. in case zvm accepted at least one of user's
 * hint it returns a new manifest.
 *
 * 2011-12-04
 * text user manifest removed. instead of it user can use UserSetup object.
 *
 * 2011-12-11
 * SystemList and Report structures removed. zvm onering wrappers updated.
 * SetupList structure updated with syscallback to support a new engine (see
 * "syscall overloading" in progress.txt
 *
 * 2012-04-22
 * user main() will be replaced with zrt main() to setup syscallback/manifest
 * and other needful things
 *
 * 2012-06-14
 * massive changes in structures and enumerations. SetupList replaced with
 * UserManifest
 *
 * 2011-11-20
 * d'b
 */
#ifndef API_ZVM_MANIFEST_H__
#define API_ZVM_MANIFEST_H__ 1

#include <stdint.h>

/* prefixes must have same order as enum ChannelType */
#define CHANNEL_PREFIXES {"Input", "Output", "UserLog", "NetInput", "NetOutput"}

/* channel mount mode */
enum MountMode {MAPPED=0, LOADED, NETWORK, INVALID=-1};

/* channels available for user. very 1st channel must be InputChannel */
enum ChannelType {
  InputChannel,
  OutputChannel,
  LogChannel,
  NetworkInputChannel,
  NetworkOutputChannel,
  ChannelTypesCount,
  EmptyChannel
};

/* todo(NETWORKING): put to this define the real channels count */
#define CHANNELS_COUNT ChannelTypesCount

/* limits for i/o */
enum IOLimits {
  GetsLimit,
  PutsLimit,
  GetSizeLimit,
  PutSizeLimit,
  IOLimitsCount
};

/* counters for i/o */
enum IOCounters {
  GetsCounter,
  PutsCounter,
  GetSizeCounter,
  PutSizeCounter,
  IOCountersCount
};

/* add new syscalls through "onering trap" here */
enum TrapCalls {
  TrapUserSetup = 17770430,
  TrapRead,
  TrapWrite,
  TrapExit
};

/* todo: rework error internal zerovm codes and put it in own header */
/* nanosleep ret codes, only 2 because of nanosleep limitations */
enum NANO_CODES {
  OK_CODE,
  PARAM_CODE, /* invalid parameter */
  SIZE_CODE, /* invalid size */
  PTR_CODE, /* invalid pointer */
  SMALL_CODE, /* too small */
  LARGE_CODE, /* too large */
  ERR_CODE = -1 /* common error */
};

/* trap error codes (addition to errno) */
enum TRAP_CODES {
  INTERNAL_ERR = 256, /* codes bellow reserved by errno */
  INVALID_DESC,
  INVALID_MODE,
  INSANE_SIZE,
  INSANE_OFFSET,
  INVALID_BUFFER,
  OUT_OF_BOUNDS,
  OUT_OF_LIMITS
};

/*
 * zerovm channel descriptor. user can access data only using channels.
 * so far zerovm only has 5 channel types and 3 mount modes
 * WARNING: any pointers must be translated to system/user space
 * note: being shared between system and user space it cannot use pointers
 *       64-bit integer should be used instead
 * todo(d'b): revise and adapt to networking
 */
struct ChannelDesc
{
  /* main part set from manifest */
  uint32_t self_size; /* size of this struct */
  /*
   *  this is a pointer converted to int because in user space
   *  pointers has different size. and this struct shared between
   *  zerovm and user
   */
  int64_t name; /* file name (real type is char*). secured */
  enum ChannelType type; /* type of channel */
  enum MountMode mounted; /* mount mode. secured */

  /* main part set in runtime */
  int32_t handle; /* file handle. secured */
  int64_t fsize; /* file size */
  int32_t buffer; /* buffer for file content. only used for mapped channels */
  int32_t bsize; /* buffer size */

  /* limits */
  int64_t limits[IOLimitsCount];
  int64_t counters[IOCountersCount];
};

/* all magic numbers about user custom attributes are here */
#define CONTENT_TYPE_LEN 64
#define TIMESTAMP_LEN 64
#define X_OBJECT_META_TAG_LEN 256
#define USER_TAG_LEN 64

/* user part of manifest. has common part with system manifest */
struct UserManifest
{
#define COMMON_PART \
  uint32_t self_size; /* size of this struct */\
  uint32_t heap_ptr; /* pointer to the start of available for user ram */\
\
  /* resources limits and counters */\
  uint32_t max_mem; /* memory space available for user program. if 0 = 4gb */\
\
  /*\
   * todo: decide whether zerovm needs to count syscalls. it already has\
   * better way to control user application: timeout, memory preallocation,\
   * channels e.t.c.\
   */\
  int32_t max_syscalls; /* max allowed *real* system calls, 0 - no limit */\
  int32_t cnt_syscalls; /* syscalls counter */\
\
  /* custom attributes. fixed length arrays, to reserve memory in the user space */\
  char content_type[CONTENT_TYPE_LEN];\
  char timestamp[TIMESTAMP_LEN]; /* time/seed for user */\
  char x_object_meta_tag[X_OBJECT_META_TAG_LEN]; /* extened user attributes */\
  char user_etag[USER_TAG_LEN]; /* user output space checksum. disabled now */\
\
  /*\
   * user controlled syscalls handler. should be provided by nexe due runtime\
   * the field has mirror in "nacl_globals.h". both variables must be set simultaniously\
   * there is a difference between them however: syscallback in this structure uses\
   * user coordinates and global syscallback point to same place but represented with\
   * system address\
   */\
  int32_t syscallback;\
\
  /* array of channels. not constructed channel has NULL in "name" */\
  int32_t channels_count; /* count of elements in the "channels" array */\
  int64_t channels; /* array of available channels (type: struct ChannelDesc*) */

  COMMON_PART /* shared with struct SystemManifest */
};

/*
 * "One Ring" wrappers and logger. disabled for trusted code
 */
#ifdef USER_SIDE

/* must be called before api usage */
struct UserManifest* zvm_init();

/* wrapper for zerovm "TrapUserSetup" */
int32_t zvm_setup(struct UserManifest *hint);

/* wrapper for zerovm "TrapRead" */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset);

/* wrapper for zerovm "TrapWrite" */
int32_t zvm_pwrite(int desc, char *buffer, int32_t size, int64_t offset);

/* wrapper for zerovm "TrapExit" */
int32_t zvm_exit(int32_t code);

/*
 * user_manifest accessors prototypes
 */

/* return user heap starting address */
void* zvm_heap_start();

/*
 * return user memory size. note that this is not the heap
 * size, but whole memory available for user
 */
uint32_t zvm_memory_size();

/* return content_type or NULL */
char* zvm_content_type();

/* return timestamp or NULL */
char* zvm_timestamp();

/* return x_object_meta_tag or NULL */
char* zvm_x_object_meta_tag();

/* return user_etag or NULL */
char* zvm_user_etag();

/* return syscallback address or 0 */
int32_t zvm_syscallback();

/* set syscallback to the new address, return -1 if failed */
int32_t zvm_set_syscallback(int32_t syscallback);

/* return memory buffer address or -1 if not available */
void* zvm_channel_addr(enum ChannelType ch);

/* return memory buffer size or -1 if not available */
int32_t zvm_channel_size(enum ChannelType ch);

/* return name of the channel or NULL. note: secured field (does not contain real name) */
char* zvm_channel_name(enum ChannelType ch);

/* return handle of the channel. note: secured field */
int32_t zvm_channel_handle(enum ChannelType ch);

/* return type of the channel */
enum ChannelType zvm_channel_type(enum ChannelType ch);

/* return mount mode of the channel. note: in a future can be secured */
enum MountMode zvm_channel_mode(enum ChannelType ch);

/* return channel file size. note: in a future can be secured */
int64_t zvm_channel_fsize(enum ChannelType ch);

/* return get size limit for the given channel */
int64_t zvm_channel_get_size_limit(enum ChannelType ch);

/* return gets count limit for the given channel */
int64_t zvm_channel_get_count_limit(enum ChannelType ch);

/* return put size limit for the given channel */
int64_t zvm_channel_put_size_limit(enum ChannelType ch);

/* return puts count limit for the given channel */
int64_t zvm_channel_put_count_limit(enum ChannelType ch);

/* return get size counter for the given channel */
int64_t zvm_channel_get_size_count(enum ChannelType ch);

/* return gets count counter for the given channel */
int64_t zvm_channel_get_count_count(enum ChannelType ch);

/* return put size counter for the given channel */
int64_t zvm_channel_put_size_count(enum ChannelType ch);

/* return puts count counter for the given channel */
int64_t zvm_channel_put_count_count(enum ChannelType ch);

#endif /* USER_SIDE */

#endif /* API_ZVM_MANIFEST_H__ */
