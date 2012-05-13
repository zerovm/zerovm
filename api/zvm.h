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
 * 2011-11-20
 * d'b
 */

#ifndef API_ZVM_MANIFEST_H__
#define API_ZVM_MANIFEST_H__ 1

#include <stdint.h> /* how to get rid of it? */

#define MAX_MANIFEST_LEN 4096

/*
 * files available for user. very 1st channel must be InputChannel
 * the last channel must be NetworkOutputChannel
 */
enum ChannelType {
  InputChannel,
  OutputChannel,
  LogChannel,
  NetworkInputChannel,
  NetworkOutputChannel
};

#define CHANNELS_COUNT (NetworkOutputChannel + 1)

/* prefixes must answer to enum "Channels" */
#define CHANNEL_PREFIXES {"Input", "Output", "UserLog", "NetInput", "NetOutput"}

/* add new syscalls through "onering trap" here */
enum TrapCalls {
  TrapUserSetup = 17770430,
  TrapRead,
  TrapWrite,
  TrapExit
};

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

/* channel mount mode */
enum MountMode {MAPPED=0, LOADED, NETWORK, INVALID=-1};

/*
 * hold information about preopened for user file
 * note: address must be translated to user space
 * note: don't use pointers in shared structures!
 */
struct PreOpenedFileDesc
{
  /* main part set from manifest */
  uint32_t self_size; /* size of this struct */
  /*
   *  this is a pointer converted to int because in user space
   *  pointers has different size. and this struct shared between
   *  zerovm and user
   */
  uint64_t name; /* file name. n/a for user */
  enum ChannelType type; /* type of channel */
  enum MountMode mounted; /* mount mode. n/a for user */

  /* main part set in runtime */
  int32_t handle; /* file handle. n/a for user */
  int64_t fsize; /* file size */
  int32_t buffer; /* buffer for file content may or may not be mapped. must be int32_t since nacl mmap */
  int32_t bsize; /* buffer size */

  /* limits */
  int64_t max_size; /* allowed channel size */
  int32_t max_gets; /* max allowed output read calls, 0 - no limit */
  int32_t max_puts; /* max allowed output write calls, 0 - no limit */
  int64_t max_get_size; /* amount of bytes allowed to read */
  int64_t max_put_size; /* amount of bytes allowed to write */

  /* counters */
  int32_t cnt_gets; /* read calls counter */
  int32_t cnt_puts; /* write calls counter */
  int64_t cnt_get_size; /* read bytes counter */
  int64_t cnt_put_size; /* written bytes counter */
};

/* all magic numbers about user custom attributes are here */
#define CONTENT_TYPE_LEN 64
#define TIMESTAMP_LEN 64
#define X_OBJECT_META_TAG_LEN 256
#define USER_TAG_LEN 64

/*
 * user policy. contain limits for user program and information need by nexe
 * note: object of this struct will be created by parser in place of manifest.text
 */
struct SetupList
{
  uint32_t self_size; /* size of this struct */
  uint32_t heap_ptr; /* pointer to the start of available for user ram */

  /* memory, cpu and other system resources limits */
  uint32_t max_mem; /* max memory space available for user program < 4gb */
  int32_t max_cpu; /* max cpu time available for user program, 0 - no limit */
  int32_t max_syscalls; /* max allowed *real* system calls, 0 - no limit */
  int32_t max_setup_calls; /* allowed calls of _trap_setup */

  /* memory, cpu and other system resources counters */
  int32_t cnt_mem; /* amount of memory available for user */
  int32_t cnt_cpu; /* n/a for user */
  int32_t cnt_cpu_last; /* n/a for user. initially should be set when nexe start */
  int32_t cnt_syscalls; /* syscalls limit */
  int32_t cnt_setup_calls;

  /* custom attributes. fixed length arrays, to reserve memory in the user space */
  char content_type[CONTENT_TYPE_LEN];
  char timestamp[TIMESTAMP_LEN]; /* time/seed for user */
  char x_object_meta_tag[X_OBJECT_META_TAG_LEN]; /* extened user attributes */
  char user_etag[USER_TAG_LEN]; /* user output space checksum. disabled now */

  /*
   * user controlled syscalls handler. should be provided by nexe due runtime
   * the field has mirror in "nacl_globals.h". both variables must be set simultaniously
   * there is a difference between them however: syscallback in this structure uses
   * user coordinates and global syscallback point to same place but represented with
   * system address
   */
  int32_t syscallback;

  /* array of channels. not constructed channel has NULL in "name" */
  struct PreOpenedFileDesc channels[CHANNELS_COUNT];
};

#ifdef USER_SIDE /* disabled for trusted code */
/* "One Ring" wrappers and logger */

/*
 * wrapper for zerovm "TrapUserSetup"
 */
int32_t zvm_setup(struct SetupList *hint);

/*
 * wrapper for zerovm "TrapRead"
 */
int32_t zvm_pread(int desc, char *buffer, int32_t size, int64_t offset);

/*
 * wrapper for zerovm "TrapWrite"
 */
int32_t zvm_pwrite(int desc, char *buffer, int32_t size, int64_t offset);

/*
 * set log (if allowed). valid SetupList object must be provided.
 */
int log_set(struct SetupList *setup);

/*
 * wrapper for zerovm "TrapExit"
 */
int32_t zvm_exit(int32_t code);

/*
 * log message. 0 - if success. 1 - if log is full or has no space to
 * store the whole message (part of the message will be stored anyway)
 */
int log_msg(char *msg);

/*
 * temporary solution for zrt library. can be removed after
 * "blob library" mechanism will be complete
 */
#if ZRT_LIB
#define main slave_main
#endif

#endif /* USER_SIDE */

#endif /* API_ZVM_MANIFEST_H__ */
