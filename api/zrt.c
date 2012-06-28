/*
 * zrt library. simple release
 *
 * this source file demonstrates "syscallback" mechanism. user can
 * install it's own handler for all syscalls (except syscall #0).
 * when installed, "syscallback" will pass control to user code whenever
 * syscalls invoked. to return control to zerovm user can use trap (syscall #0)
 * or uninstall "syscallback" (samples how to do it can be found in this code
 * as well).
 *
 * this example can (and hopefully will) be extended to zrt library. information
 * about that library will be placed to zerovm website
 *
 * to extend this example just write code of all zrt_* functions. these functions
 * can use any of zerovm appliances for user code (like input stream, output stream, log,
 * extended attributes e.t.c). any of this can be obtained from the user manifest.
 *
 * note: direct syscalls defined here not only for test purposes. direct nacl syscalls
 *       can be used for "selective syscallback" - this is allow to intercept particular
 *       syscalls while leaving other syscalls to zerovm care (see zrt_mmap()).
 *
 *  Created on: Feb 18, 2012
 *      Author: d'b
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "api/zvm.h"
#undef main /* prevent misuse macro */

#include <unistd.h> /* only for tests */

/*
 * temporary fix for nacl. stat != nacl_abi_stat
 * also i had a weird error when tried to use "service_runtime/include/sys/stat.h"
 */
struct nacl_abi_stat
{
  /* must be renamed when ABI is exported */
  int64_t   nacl_abi_st_dev;       /* not implemented */
  uint64_t  nacl_abi_st_ino;       /* not implemented */
  uint32_t  nacl_abi_st_mode;      /* partially implemented. */
  uint32_t  nacl_abi_st_nlink;     /* link count */
  uint32_t  nacl_abi_st_uid;       /* not implemented */
  uint32_t  nacl_abi_st_gid;       /* not implemented */
  int64_t   nacl_abi_st_rdev;      /* not implemented */
  int64_t   nacl_abi_st_size;      /* object size */
  int32_t   nacl_abi_st_blksize;   /* not implemented */
  int32_t   nacl_abi_st_blocks;    /* not implemented */
  int64_t   nacl_abi_st_atime;     /* access time */
  int64_t   nacl_abi_st_atimensec; /* possibly just pad */
  int64_t   nacl_abi_st_mtime;     /* modification time */
  int64_t   nacl_abi_st_mtimensec; /* possibly just pad */
  int64_t   nacl_abi_st_ctime;     /* inode change time */
  int64_t   nacl_abi_st_ctimensec; /* possibly just pad */
};

/* same here */
struct nacl_abi_timeval {
  int64_t   nacl_abi_tv_sec;
  int32_t   nacl_abi_tv_usec;
};

/* entry point for zrt library sample (see "syscall_manager.S" file) */
void syscall_director(void);

/* need to be here for selective syscallback */
/*
 * user manifest object. should be initialized before any usage.
 * memory for the channels should be allocated
 * todo(d'b): how to get number of channels?
 */
struct UserManifest setup;
#define MAX_CHANNELS 1024
char channels_space_holder[MAX_CHANNELS * sizeof(struct ChannelDesc)];

/* positions of zerovm streams */
static size_t pos_ptr[ChannelTypesCount] = {0};

/* current end of memory (sysbreak). must be initialized from zrt_main() */
static void *cur_break = NULL;

/* NOTE: debug purposes only!
 * log message. needs valid initialized setup
 * replaces obsolete function from "zvm.c"
 */
int log_msg(char *msg)
{
  struct ChannelDesc *channel;
  int32_t length;

	if (msg == NULL) return ERR_CODE;
	length = (msg == NULL) ? 0 : strlen(msg);

	channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[LogChannel];

	/* check if lenght is not overrun available data */
	if(pos_ptr[LogChannel] + length > channel->bsize)
		length = channel->bsize - pos_ptr[LogChannel];

	if(length < 0) return ERR_CODE;

	memcpy((void*)channel->buffer + pos_ptr[LogChannel], msg, length);
	pos_ptr[LogChannel] += length;

	return OK_CODE;
}

#define DEBUG 0

#if DEBUG
#define SHOWID do {log_msg((char*)__func__); log_msg("() is called\n");} while(0)
#else
#define SHOWID
#endif

/*
 * FULL NACL SYSCALL LIST
 * should be moved to the header file
 */
#define NACL_sys_null                    1 /* empty syscall. does nothing */
#define NACL_sys_nameservice             2
#define NACL_sys_dup                     8
#define NACL_sys_dup2                    9
#define NACL_sys_open                   10
#define NACL_sys_close                  11
#define NACL_sys_read                   12
#define NACL_sys_write                  13
#define NACL_sys_lseek                  14
#define NACL_sys_ioctl                  15
#define NACL_sys_stat                   16
#define NACL_sys_fstat                  17
#define NACL_sys_chmod                  18
#define NACL_sys_sysbrk                 20
#define NACL_sys_mmap                   21
#define NACL_sys_munmap                 22
#define NACL_sys_getdents               23
#define NACL_sys_exit                   30
#define NACL_sys_getpid                 31
#define NACL_sys_sched_yield            32
#define NACL_sys_sysconf                33
#define NACL_sys_gettimeofday           40
#define NACL_sys_clock                  41
#define NACL_sys_nanosleep              42
#define NACL_sys_imc_makeboundsock      60
#define NACL_sys_imc_accept             61
#define NACL_sys_imc_connect            62
#define NACL_sys_imc_sendmsg            63
#define NACL_sys_imc_recvmsg            64
#define NACL_sys_imc_mem_obj_create     65
#define NACL_sys_imc_socketpair         66
#define NACL_sys_mutex_create           70
#define NACL_sys_mutex_lock             71
#define NACL_sys_mutex_trylock          72
#define NACL_sys_mutex_unlock           73
#define NACL_sys_cond_create            74
#define NACL_sys_cond_wait              75
#define NACL_sys_cond_signal            76
#define NACL_sys_cond_broadcast         77
#define NACL_sys_cond_timed_wait_abs    79
#define NACL_sys_thread_create          80
#define NACL_sys_thread_exit            81
#define NACL_sys_tls_init               82
#define NACL_sys_thread_nice            83
#define NACL_sys_tls_get                84
#define NACL_sys_second_tls_set         85
#define NACL_sys_second_tls_get         86
#define NACL_sys_sem_create             100
#define NACL_sys_sem_wait               101
#define NACL_sys_sem_post               102
#define NACL_sys_sem_get_value          103
#define NACL_sys_dyncode_create         104
#define NACL_sys_dyncode_modify         105
#define NACL_sys_dyncode_delete         106
#define NACL_sys_test_infoleak          109

/*
 * DIRECT NACL SYSCALLS FUNCTIONS (VIA TRAMPOLINE)
 * should be fulfilled with all syscalls and moved to the header.
 */
/* write() -- nacl syscall via trampoline */
#define NaCl_write(d, buf, count) ((int32_t (*)(uint32_t, uint32_t, uint32_t)) \
    (NACL_sys_write * 0x20 + 0x10000))(d, buf, count)

/* mmap() -- nacl syscall via trampoline */
#define NaCl_mmap(start, length, prot, flags, d, offp) \
  ((int32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)) \
    (NACL_sys_mmap * 0x20 + 0x10000))(start, length, prot, flags, d, offp)

/* munmap() -- nacl syscall via trampoline */
#define NaCl_munmap(start, length) ((int32_t (*)(uint32_t, uint32_t)) \
    (NACL_sys_munmap * 0x20 + 0x10000))(start, length)

/* sysbrk() -- nacl syscall via trampoline */
#define NaCl_sysbrk(new_break) ((int32_t (*)(uint32_t)) \
    (NACL_sys_sysbrk * 0x20 + 0x10000))(new_break)

/* tls_get() -- nacl syscall via trampoline */
#define NaCl_tls_get() ((int32_t (*)()) \
    (NACL_sys_tls_get * 0x20 + 0x10000))()

/* invalid syscall */
#define NaCl_invalid() ((int32_t (*)()) \
    (999 * 0x20 + 0x10000))()

/*
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 */

/* dummy for not implemented syscalls */
int32_t zrt_nan(uint32_t *args)
{
  /* log/return "not implemented syscall" error */
  SHOWID; return 0;
}

/* empty syscall. does nothing */
int32_t zrt_null(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_nameservice(uint32_t *args)
{
  SHOWID; return 0;
}

/*
 * duplicate the given file handle
 * not allowed in simple zrt version
 */
int32_t zrt_dup(uint32_t *args)
{
  SHOWID; return -EPERM; /* Operation is not permitted */
}

/*
 * duplicate the given file handle
 * not allowed in simple zrt version
 */
int32_t zrt_dup2(uint32_t *args)
{
  SHOWID;
  return -EPERM; /* Operation is not permitted */
}

/* open the file with the given handle number */
int32_t zrt_open(uint32_t *args)
{
  SHOWID;

  /*
   * in the simple version of zrt library we only can use stdin, stdout and stderr
   * (input, output, user_log) which are already opened or not available. so we
   * can just fail on this function
   */
  return -ENOENT; /* No such file or directory */
}

/* close the file with the given handle number */
int32_t zrt_close(uint32_t *args)
{
  /*
   * in the simple version of zrt library closing of stream
   * is not allowed (since it makes no sense). so we just fail
   */
  SHOWID; return -EBADF;
}

/* read the file with the given handle number */
int32_t zrt_read(uint32_t *args)
{
  /*
   * in the simple zrt library version we only can read
   * stdin == 0, stdout ==  1 and stderr == 2 (wich are really
   * input, output and user_log)
   */
  SHOWID;
  int file = (int)args[0];
  void *buf = (void*)args[1];
  int64_t length = (int64_t)args[2];
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup.channels;

  /*
   * Support for MSQ files: zvm_pread used for networking communication
   * msq files has descriptor numbers above than 2
   * todo(NETWORKING): move it to switch after networking integration will be complete
   */
  /* MSQ files using streaming IO, and don't using offset, set offset as 0 */
  if(file > LogChannel)
    return zvm_pread(file, (void*) args[1], length, 0);

  /* check given handle. check length */
  if( InputChannel != file) return -EBADF;
  if(length < 0 || length > 0x7fffffff) return -EPERM;

  /* write data */
  switch(channels[file].mounted)
  {
    case MAPPED:
      /* check if length is not overrun available data */
      if(pos_ptr[file] + length > channels[file].bsize)
        length = channels[file].bsize - pos_ptr[file];

      /* length must not be negative */
      if(length < 0)
      {
        length = -EBADF;
        break;
      }

      memcpy(buf, (void*)channels[file].buffer + pos_ptr[file], length);
      pos_ptr[file] += length;
      break;

    case LOADED:
      length = zvm_pread(file, buf, length, pos_ptr[file]);
      if(length > 0) pos_ptr[file] += length;
      break;

    default: /* the mounting method is not supported */
      length = -EBADF;
      break;
  }

  return length;
}

/* example how to implement zrt syscall */
int32_t zrt_write(uint32_t *args)
{
  /*
   * in the simple zrt library version we only can write
   * stdout ==  1 and stderr == 2 (wich are really
   * output and user_log). stdin == 0 is not allowed to write
   */
  SHOWID;
  int file = (int)args[0];
  void *buf = (void*)args[1];
  int64_t length = (int64_t)args[2];
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup.channels;

  /*
   * Support for MSQ files: zvm_pwrite used for networking communication
   * todo(NETWORKING): move it to switch after networking integration will be complete
   */
  /* MSQ files using streaming IO, and don't using offset, set offset as 0 */
  if(file > LogChannel)
    return zvm_pwrite(file, buf, length, 0);

  /* check given handle. check length */
  if(file < OutputChannel || file > LogChannel) return -EBADF;
  if(length < 0 || length > 0x7fffffff) return -EPERM;

  /* write data */
  switch(channels[file].mounted)
  {
    case MAPPED:
      /* check if length is not overrun available data */
      if(pos_ptr[file] + length > channels[file].bsize)
        length = channels[file].bsize - pos_ptr[file];

      /* length must not be negative */
      if(length < 0)
      {
        length = -EBADF;
        break;
      }

      memcpy((void*)channels[file].buffer + pos_ptr[file], buf, length);
      pos_ptr[file] += length;
      break;

    case LOADED:
      length = zvm_pwrite(file, buf, length, pos_ptr[file]);
      if(length > 0) pos_ptr[file] += length;
      break;

    default: /* the mounting method is not supported */
      length = -EBADF;
      break;
  }

  return length;
}

/*
 * seek position does not work for stdin/stdout/stderr
 * so we just fail in simple version of zrt
 * note: actualy we can position, we just don't want to enhance standard
 * UPDATE: seek temporary allowed
 */
int32_t zrt_lseek(uint32_t *args)
{
#define CHECK_NEW_POS(offset)\
  if(offset < 0 || offset > 0x7fffffff)\
  {\
    errno = EPERM; /* in advanced version should be set to conventional value */\
    return -EPERM;\
  }

  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup.channels;

  SHOWID;
  enum ChannelType handle = (enum ChannelType)args[0];
  off_t offset = *((off_t*)args[1]);
  int whence = (int)args[2];
  off_t new_pos;

  /* check if given handle is valid and seekable */
  if(handle < InputChannel || handle > LogChannel) return -EBADF;

  switch(whence)
  {
    case SEEK_SET:
      CHECK_NEW_POS(offset);
      pos_ptr[handle] = offset;
      break;
    case SEEK_CUR:
      new_pos = pos_ptr[handle] + offset;
      CHECK_NEW_POS(new_pos);
      pos_ptr[handle] = new_pos;
      break;
    case SEEK_END:
      new_pos = channels[handle].fsize + offset;
      CHECK_NEW_POS(new_pos);
      pos_ptr[handle] = new_pos;
      break;
    default:
      errno = EPERM; /* in advanced version should be set to conventional value */
      return -EPERM;
  }

  /*
   * return current position in a special way since 64 bits
   * doesn't fit to return code (32 bits)
   */
  *(off_t *)args[1] = pos_ptr[handle];
  return 0;
}

/* low level calls are not implemented in the simple version of zrtlib */
int32_t zrt_ioctl(uint32_t *args)
{
  SHOWID; return -EINVAL;
}

/* return synthetic channel information */
int32_t zrt_stat(uint32_t *args)
{
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup.channels;

  SHOWID;
  char *prefixes[] = CHANNEL_PREFIXES;
  const char *file = (const char*)args[0];
  struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
  enum ChannelType handle;

  /* ensure "file" is not NULL. calculate handle number */
  if(file == NULL) return -EFAULT;
  for(handle = InputChannel; handle < sizeof(prefixes)/sizeof(*prefixes); ++handle)
      if(!strcmp(file, prefixes[0])) break;

  /*
   * check if user request contain the proper file name:
   * stdin == Input, stdout == Output, stderr == UserLog
   * note: input, output, log are default zerovm streams names
   */
  if(handle >= InputChannel && handle <= LogChannel)
  {
    /* return stat object */
    sbuf->nacl_abi_st_dev = 2049;     /* ID of device containing handle */
    sbuf->nacl_abi_st_ino = 1967;     /* inode number */
    sbuf->nacl_abi_st_mode = 33261;   /* protection */
    sbuf->nacl_abi_st_nlink = 1;      /* number of hard links */
    sbuf->nacl_abi_st_uid = 1000;     /* user ID of owner */
    sbuf->nacl_abi_st_gid = 1000;     /* group ID of owner */
    sbuf->nacl_abi_st_rdev = 0;       /* device ID (if special handle) */
    sbuf->nacl_abi_st_size = channels[handle].fsize;    /* total size, in bytes */
    sbuf->nacl_abi_st_blksize = 4096; /* blocksize for file system I/O */
    /* number of 512B blocks allocated */
    sbuf->nacl_abi_st_blocks = ((sbuf->nacl_abi_st_size + sbuf->nacl_abi_st_blksize - 1) /
        sbuf->nacl_abi_st_blksize) * sbuf->nacl_abi_st_blksize / 512;

    /*
     * we are not allowed to have real date/time. for streams
     * we can use any constant or timestamp from manifest (if available)
     */
    sbuf->nacl_abi_st_atime = 0;      /* time of last access */
    sbuf->nacl_abi_st_mtime = 0;      /* time of last modification */
    sbuf->nacl_abi_st_ctime = 0;      /* time of last status change */
    sbuf->nacl_abi_st_atimensec = 0;
    sbuf->nacl_abi_st_mtimensec = 0;
    sbuf->nacl_abi_st_ctimensec = 0;

    return 0;
  }
  return -ENOENT;
}

/* return synthetic channel information */
int32_t zrt_fstat(uint32_t *args)
{
  SHOWID;
  enum ChannelType handle = (int)args[0];
  struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup.channels;

  /*
   * check if user request contain the proper file handle:
   * stdin == 0, stdout == 1, stderr == 2
   */
  if(handle < InputChannel || handle > LogChannel) return -EBADF;

  /* return stat object */
    sbuf->nacl_abi_st_dev = 2049;     /* ID of device containing handle */
    sbuf->nacl_abi_st_ino = 1967;     /* inode number */
    sbuf->nacl_abi_st_mode = 33261;   /* protection */
    sbuf->nacl_abi_st_nlink = 1;      /* number of hard links */
    sbuf->nacl_abi_st_uid = 1000;     /* user ID of owner */
    sbuf->nacl_abi_st_gid = 1000;     /* group ID of owner */
    sbuf->nacl_abi_st_rdev = 0;       /* device ID (if special handle) */
    sbuf->nacl_abi_st_size = channels[handle].fsize;    /* total size, in bytes */
    sbuf->nacl_abi_st_blksize = 4096; /* blocksize for file system I/O */
    /* number of 512B blocks allocated */
    sbuf->nacl_abi_st_blocks = ((sbuf->nacl_abi_st_size + sbuf->nacl_abi_st_blksize - 1) /
        sbuf->nacl_abi_st_blksize) * sbuf->nacl_abi_st_blksize / 512;

    /*
     * we are not allowed to have real date/time. for streams
     * we can use any constant or timestamp from manifest (if available)
     */
    sbuf->nacl_abi_st_atime = 0;      /* time of last access */
    sbuf->nacl_abi_st_mtime = 0;      /* time of last modification */
    sbuf->nacl_abi_st_ctime = 0;      /* time of last status change */
    sbuf->nacl_abi_st_atimensec = 0;
    sbuf->nacl_abi_st_mtimensec = 0;
    sbuf->nacl_abi_st_ctimensec = 0;

  return 0;
}

/* in a simple version of zrt chmod is not allowed */
int32_t zrt_chmod(uint32_t *args)
{
  SHOWID;
  return -EPERM;
}

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 * zrt lib will help user to do it transparently.
 */

/* change space allocation */
int32_t zrt_sysbrk(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);

  /* invoke syscall directly */
  retcode = NaCl_sysbrk(args[0]);

  /* reinstall syscallback */
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);

  return retcode;
}

/* map region of memory */
int32_t zrt_mmap(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);

  /* invoke syscall directly */
  retcode = NaCl_mmap(args[0], args[1], args[2], args[3], args[4], args[5]);

  /* reinstall syscallback */
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);

  return retcode;
}

/*
 * unmap region of memory
 * note: zerovm doesn't use it in memory management.
 * instead of munmap it use mmap with protection 0
 */
int32_t zrt_munmap(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);

  /* invoke syscall directly */
  retcode = NaCl_munmap(args[0], args[1]);

  /* reinstall syscallback */
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);

  return retcode;
}

/* mock. should be implemented */
int32_t zrt_getdents(uint32_t *args)
{
  SHOWID; return 0;
}

/*
 * exit. most important syscall. without it the user program
 * cannot terminate correctly.
 */
int32_t zrt_exit(uint32_t *args)
{
  /* no need to check args for NULL. it is always set by syscall_manager */
  SHOWID;
  zvm_exit(args[0]);

  /* not reached */
  return 0;
}

/* mock. should be implemented */
int32_t zrt_getpid(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sched_yield(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sysconf(uint32_t *args)
{
  SHOWID; return 0;
}

/* if given in manifest let user to have it */
int32_t zrt_gettimeofday(uint32_t *args)
{
  struct nacl_abi_timeval  *tv = (struct nacl_abi_timeval *)args[0];
  char *stamp = (char *)setup.timestamp;
  SHOWID;

  /* check if timestampr is set */
  if(!*stamp) return -EPERM;

  /* check given arguments validity */
  if(!tv) return -EFAULT;

  tv->nacl_abi_tv_usec = 0; /* to simplify code. yet we can't get msec from nacl code */
  tv->nacl_abi_tv_sec = atoi(stamp); /* manifest always contain decimal values */

  return 0;
}

/*
 * should never be implemented if we want deterministic behaviour
 * note: but we can allow to return each time synthetic value
 * warning! after checking i found that nacl is not using it, so this
 *          function is useless for current nacl sdk version.
 */
int32_t zrt_clock(uint32_t *args)
{
  SHOWID; return -EPERM;
}

/* sleep for for deterministic processes */
int32_t zrt_nanosleep(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_makeboundsock(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_accept(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_connect(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_sendmsg(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_recvmsg(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_mem_obj_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_imc_socketpair(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_mutex_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_mutex_lock(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_mutex_trylock(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_mutex_unlock(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_cond_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_cond_wait(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_cond_signal(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_cond_broadcast(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_cond_timed_wait_abs(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_thread_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_thread_exit(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_tls_init(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_thread_nice(uint32_t *args)
{
  SHOWID; return 0;
}

/*
 * get tls from zerovm. can be replaced with untrusted version
 * after "blob library" loader will be ready
 */
int32_t zrt_tls_get(uint32_t *args)
{
  //SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);

  /* invoke syscall directly */
  retcode = NaCl_tls_get();

  /* reinstall syscallback */
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);

  return retcode;
}

/* mock. should be implemented */
int32_t zrt_second_tls_set(uint32_t *args)
{
  SHOWID; return 0;
}

/*
 * get second tls.
 * since we only have single thread use instead of 2nd tls 1st one
 */
int32_t zrt_second_tls_get(uint32_t *args)
{
  SHOWID;
  return zrt_tls_get(NULL);
}

/* mock. should be implemented */
int32_t zrt_sem_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sem_wait(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sem_post(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sem_get_value(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_dyncode_create(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_dyncode_modify(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_dyncode_delete(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_test_infoleak(uint32_t *args)
{
  SHOWID; return 0;
}

/*
 * array of the pointers to zrt syscalls
 *
 * each zrt function (syscall) has uniform prototype: int32_t syscall(uint32_t *args)
 * where "args" pointer to syscalls' arguments. number and types of arguments
 * can be found in the nacl "nacl_syscall_handlers.c" file.
 */
int32_t (*zrt_syscalls[])(uint32_t*) = {
    zrt_nan,                   /* 0 -- not implemented syscall */
    zrt_null,                  /* 1 -- empty syscall. does nothing */
    zrt_nameservice,           /* 2 */
    zrt_nan,                   /* 3 -- not implemented syscall */
    zrt_nan,                   /* 4 -- not implemented syscall */
    zrt_nan,                   /* 5 -- not implemented syscall */
    zrt_nan,                   /* 6 -- not implemented syscall */
    zrt_nan,                   /* 7 -- not implemented syscall */
    zrt_dup,                   /* 8 */
    zrt_dup2,                  /* 9 */
    zrt_open,                  /* 10 */
    zrt_close,                 /* 11 */
    zrt_read,                  /* 12 */
    zrt_write,                 /* 13 */
    zrt_lseek,                 /* 14 */
    zrt_ioctl,                 /* 15 */
    zrt_stat,                  /* 16 */
    zrt_fstat,                 /* 17 */
    zrt_chmod,                 /* 18 */
    zrt_nan,                   /* 19 -- not implemented syscall */
    zrt_sysbrk,                /* 20 */
    zrt_mmap,                  /* 21 */
    zrt_munmap,                /* 22 */
    zrt_getdents,              /* 23 */
    zrt_nan,                   /* 24 -- not implemented syscall */
    zrt_nan,                   /* 25 -- not implemented syscall */
    zrt_nan,                   /* 26 -- not implemented syscall */
    zrt_nan,                   /* 27 -- not implemented syscall */
    zrt_nan,                   /* 28 -- not implemented syscall */
    zrt_nan,                   /* 29 -- not implemented syscall */
    zrt_exit,                  /* 30 -- must use trap:exit() */
    zrt_getpid,                /* 31 */
    zrt_sched_yield,           /* 32 */
    zrt_sysconf,               /* 33 */
    zrt_nan,                   /* 34 -- not implemented syscall */
    zrt_nan,                   /* 35 -- not implemented syscall */
    zrt_nan,                   /* 36 -- not implemented syscall */
    zrt_nan,                   /* 37 -- not implemented syscall */
    zrt_nan,                   /* 38 -- not implemented syscall */
    zrt_nan,                   /* 39 -- not implemented syscall */
    zrt_gettimeofday,          /* 40 */
    zrt_clock,                 /* 41 */
    zrt_nanosleep,             /* 42 */
    zrt_nan,                   /* 43 -- not implemented syscall */
    zrt_nan,                   /* 44 -- not implemented syscall */
    zrt_nan,                   /* 45 -- not implemented syscall */
    zrt_nan,                   /* 46 -- not implemented syscall */
    zrt_nan,                   /* 47 -- not implemented syscall */
    zrt_nan,                   /* 48 -- not implemented syscall */
    zrt_nan,                   /* 49 -- not implemented syscall */
    zrt_nan,                   /* 50 -- not implemented syscall */
    zrt_nan,                   /* 51 -- not implemented syscall */
    zrt_nan,                   /* 52 -- not implemented syscall */
    zrt_nan,                   /* 53 -- not implemented syscall */
    zrt_nan,                   /* 54 -- not implemented syscall */
    zrt_nan,                   /* 55 -- not implemented syscall */
    zrt_nan,                   /* 56 -- not implemented syscall */
    zrt_nan,                   /* 57 -- not implemented syscall */
    zrt_nan,                   /* 58 -- not implemented syscall */
    zrt_nan,                   /* 59 -- not implemented syscall */
    zrt_imc_makeboundsock,     /* 60 */
    zrt_imc_accept,            /* 61 */
    zrt_imc_connect,           /* 62 */
    zrt_imc_sendmsg,           /* 63 */
    zrt_imc_recvmsg,           /* 64 */
    zrt_imc_mem_obj_create,    /* 65 */
    zrt_imc_socketpair,        /* 66 */
    zrt_nan,                   /* 67 -- not implemented syscall */
    zrt_nan,                   /* 68 -- not implemented syscall */
    zrt_nan,                   /* 69 -- not implemented syscall */
    zrt_mutex_create,          /* 70 */
    zrt_mutex_lock,            /* 71 */
    zrt_mutex_trylock,         /* 72 */
    zrt_mutex_unlock,          /* 73 */
    zrt_cond_create,           /* 74 */
    zrt_cond_wait,             /* 75 */
    zrt_cond_signal,           /* 76 */
    zrt_cond_broadcast,        /* 77 */
    zrt_nan,                   /* 78 -- not implemented syscall */
    zrt_cond_timed_wait_abs,   /* 79 */
    zrt_thread_create,         /* 80 */
    zrt_thread_exit,           /* 81 */
    zrt_tls_init,              /* 82 */
    zrt_thread_nice,           /* 83 */
    zrt_tls_get,               /* 84 */
    zrt_second_tls_set,        /* 85 */
    zrt_second_tls_get,        /* 86 */
    zrt_nan,                   /* 87 -- not implemented syscall */
    zrt_nan,                   /* 88 -- not implemented syscall */
    zrt_nan,                   /* 89 -- not implemented syscall */
    zrt_nan,                   /* 90 -- not implemented syscall */
    zrt_nan,                   /* 91 -- not implemented syscall */
    zrt_nan,                   /* 92 -- not implemented syscall */
    zrt_nan,                   /* 93 -- not implemented syscall */
    zrt_nan,                   /* 94 -- not implemented syscall */
    zrt_nan,                   /* 95 -- not implemented syscall */
    zrt_nan,                   /* 96 -- not implemented syscall */
    zrt_nan,                   /* 97 -- not implemented syscall */
    zrt_nan,                   /* 98 -- not implemented syscall */
    zrt_nan,                   /* 99 -- not implemented syscall */
    zrt_sem_create,            /* 100 */
    zrt_sem_wait,              /* 101 */
    zrt_sem_post,              /* 102 */
    zrt_sem_get_value,         /* 103 */
    zrt_dyncode_create,        /* 104 */
    zrt_dyncode_modify,        /* 105 */
    zrt_dyncode_delete,        /* 106 */
    zrt_nan,                   /* 107 -- not implemented syscall */
    zrt_nan,                   /* 108 -- not implemented syscall */
    zrt_test_infoleak          /* 109 */
};

/*
 * UserManifest data accessors
 */

/* return content_type or NULL */
char* zvm_content_type()
{
  return *setup.content_type ? setup.content_type : NULL;
}

/* return timestamp or NULL */
char* zvm_timestamp()
{
  return *setup.timestamp ? setup.timestamp : NULL;
}

/* return x_object_meta_tag or NULL */
char* zvm_x_object_meta_tag()
{
  return *setup.x_object_meta_tag ? setup.x_object_meta_tag : NULL;
}

/* return user_etag or NULL */
char* zvm_user_etag()
{
  return *setup.user_etag ? setup.user_etag : NULL;
}

/* return syscallback address or 0 */
int32_t zvm_syscallback()
{
  return setup.syscallback;
}

/* set syscallback to the new address, return -1 if failed */
int32_t zvm_set_syscallback(int32_t syscallback)
{
  setup.syscallback = syscallback;
  return zvm_setup(&setup);
}

/* return memory buffer address or -1 if not available */
void* zvm_channel_addr(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return (void*)channel->buffer;
}

/* return memory buffer size or -1 if not available */
int32_t zvm_channel_size(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
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
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->name ? (char*)(intptr_t)channel->name : names[(int)ch];
  /* (int)ch cast to prevent compiler warning */
}

/* return handle of the channel. note: secured field */
int32_t zvm_channel_handle(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->handle;
}

/* return type of the channel */
enum ChannelType zvm_channel_type(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->type;
}

/* return mount mode of the channel. note: in a future can be secured */
enum MountMode zvm_channel_mode(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->mounted;
}

/* return channel file size. note: in a future can be secured */
int64_t zvm_channel_fsize(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->fsize;
}

/* return get size limit for the given channel */
int64_t zvm_channel_get_size_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->limits[GetSizeLimit];
}

/* return gets count limit for the given channel */
int64_t zvm_channel_get_count_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->limits[GetsLimit];
}

/* return put size limit for the given channel */
int64_t zvm_channel_put_size_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->limits[PutSizeLimit];
}

/* return puts count limit for the given channel */
int64_t zvm_channel_put_count_limit(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->limits[PutsLimit];
}

/* return get size counter for the given channel */
int64_t zvm_channel_get_size_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->counters[GetSizeCounter];
}

/* return gets count counter for the given channel */
int64_t zvm_channel_get_count_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->counters[GetsCounter];
}

/* return put size counter for the given channel */
int64_t zvm_channel_put_size_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->counters[PutSizeCounter];
}

/* return puts count counter for the given channel */
int64_t zvm_channel_put_count_count(enum ChannelType ch)
{
  struct ChannelDesc *channel;
  channel = &((struct ChannelDesc*)(intptr_t)setup.channels)[ch];
  return channel->counters[PutsCounter];
}

int slave_main(int argc, char **argv);

int main(int argc, char **argv)
{
  int retcode = ERR_CODE;

  /*
   * initialize setup (space in the stack is already allocated)
   * get user manifest, install syscallback.
   */
  setup.channels = (intptr_t)&channels_space_holder;
  setup.syscallback = (int32_t) syscall_director;
  retcode = zvm_setup(&setup);
  if(retcode) return retcode;

  /* setup user memory */
  cur_break = (void*)setup.heap_ptr;

  /* call user main() and care about return code */
  return slave_main(argc, argv);
}
