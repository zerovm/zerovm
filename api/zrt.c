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
#include <unistd.h> /* only for tests */

#include "zvm.h"
#include "zrt.h"

// ### revise it
#undef main /* prevent misuse macro */

/* pointer to the user manifest object */
static struct UserManifest *setup;

/* positions of zerovm streams */
static size_t pos_ptr[ChannelTypesCount] = {0};

/*
 * ZRT IMPLEMENTATION OF NACL SYSCALLS
 * each nacl syscall must be implemented or, at least, mocked. no exclusions!
 */

SYSCALL_MOCK(nan, 0)
SYSCALL_MOCK(null, 0)
SYSCALL_MOCK(nameservice, 0)
SYSCALL_MOCK(dup, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */
SYSCALL_MOCK(dup2, -EPERM) /* duplicate the given file handle. n/a in the simple zrt version */
SYSCALL_MOCK(open, -ENOENT) /* open the file with the given handle number */
SYSCALL_MOCK(close, -EBADF) /* close the file with the given handle number */

/* read the file with the given handle number */
static int32_t zrt_read(uint32_t *args)
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
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup->channels;

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
static int32_t zrt_write(uint32_t *args)
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
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup->channels;

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
 * note: actually we can position, we just don't want to enhance standard
 * UPDATE: seek temporary allowed
 */
static int32_t zrt_lseek(uint32_t *args)
{
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup->channels;
  SHOWID;

  #define CHECK_NEW_POS(offset)\
  if(offset < 0 || offset > 0x7fffffff)\
  {\
    errno = EPERM; /* in advanced version should be set to conventional value */\
    return -EPERM;\
  }

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

SYSCALL_MOCK(ioctl, -EINVAL) /* not implemented in the simple version of zrtlib */

/* return synthetic channel information */
static int32_t zrt_stat(uint32_t *args)
{
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup->channels;

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
    sbuf->nacl_abi_st_blksize = 4096; /* block size for file system I/O */
    /* number of 512B blocks allocated */
    sbuf->nacl_abi_st_blocks = ((sbuf->nacl_abi_st_size + sbuf->nacl_abi_st_blksize - 1) /
        sbuf->nacl_abi_st_blksize) * sbuf->nacl_abi_st_blksize / 512;

    /*
     * we are not allowed to have real date/time. for streams
     * we can use any constant or time stamp from manifest (if available)
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
static int32_t zrt_fstat(uint32_t *args)
{
  SHOWID;
  enum ChannelType handle = (int)args[0];
  struct nacl_abi_stat *sbuf = (struct nacl_abi_stat *)args[1];
  struct ChannelDesc *channels = (struct ChannelDesc*)(intptr_t)setup->channels;

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

SYSCALL_MOCK(chmod, -EPERM) /* in a simple version of zrt chmod is not allowed */

/*
 * following 3 functions (sysbrk, mmap, munmap) is the part of the
 * new memory engine. the new allocate specified amount of ram before
 * user code start and then user can only obtain that allocated memory.
 * zrt lib will help user to do it transparently.
 */

/* change space allocation */
static int32_t zrt_sysbrk(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup->syscallback = 0;
  zvm_setup(setup);

  /* invoke syscall directly */
  retcode = NaCl_sysbrk(args[0]);

  /* reinstall syscallback */
  setup->syscallback = (int32_t) syscall_director;
  zvm_setup(setup);

  return retcode;
}

/* map region of memory */
static int32_t zrt_mmap(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup->syscallback = 0;
  zvm_setup(setup);

  /* invoke syscall directly */
  retcode = NaCl_mmap(args[0], args[1], args[2], args[3], args[4], args[5]);

  /* reinstall syscallback */
  setup->syscallback = (int32_t) syscall_director;
  zvm_setup(setup);

  return retcode;
}

/*
 * unmap region of memory
 * note: zerovm doesn't use it in memory management.
 * instead of munmap it use mmap with protection 0
 */
static int32_t zrt_munmap(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup->syscallback = 0;
  zvm_setup(setup);

  /* invoke syscall directly */
  retcode = NaCl_munmap(args[0], args[1]);

  /* reinstall syscallback */
  setup->syscallback = (int32_t) syscall_director;
  zvm_setup(setup);

  return retcode;
}

SYSCALL_MOCK(getdents, 0)

/*
 * exit. most important syscall. without it the user program
 * cannot terminate correctly.
 */
static int32_t zrt_exit(uint32_t *args)
{
  /* no need to check args for NULL. it is always set by syscall_manager */
  SHOWID;
  zvm_exit(args[0]);

  /* not reached */
  return 0;
}

SYSCALL_MOCK(getpid, 0)
SYSCALL_MOCK(sched_yield, 0)
SYSCALL_MOCK(sysconf, 0)

/* if given in manifest let user to have it */
static int32_t zrt_gettimeofday(uint32_t *args)
{
  struct nacl_abi_timeval  *tv = (struct nacl_abi_timeval *)args[0];
  char *stamp = (char *)setup->timestamp;
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
SYSCALL_MOCK(clock, -EPERM)
SYSCALL_MOCK(nanosleep, 0)
SYSCALL_MOCK(imc_makeboundsock, 0)
SYSCALL_MOCK(imc_accept, 0)
SYSCALL_MOCK(imc_connect, 0)
SYSCALL_MOCK(imc_sendmsg, 0)
SYSCALL_MOCK(imc_recvmsg, 0)
SYSCALL_MOCK(imc_mem_obj_create, 0)
SYSCALL_MOCK(imc_socketpair, 0)
SYSCALL_MOCK(mutex_create, 0)
SYSCALL_MOCK(mutex_lock, 0)
SYSCALL_MOCK(mutex_trylock, 0)
SYSCALL_MOCK(mutex_unlock, 0)
SYSCALL_MOCK(cond_create, 0)
SYSCALL_MOCK(cond_wait, 0)
SYSCALL_MOCK(cond_signal, 0)
SYSCALL_MOCK(cond_broadcast, 0)
SYSCALL_MOCK(cond_timed_wait_abs, 0)
SYSCALL_MOCK(thread_create, 0)
SYSCALL_MOCK(thread_exit, 0)
SYSCALL_MOCK(tls_init, 0)
SYSCALL_MOCK(thread_nice, 0)

/*
 * get tls from zerovm. can be replaced with untrusted version
 * after "blob library" loader will be ready
 */
static int32_t zrt_tls_get(uint32_t *args)
{
  SHOWID;
  int32_t retcode;

  /* uninstall syscallback */
  setup->syscallback = 0;
  zvm_setup(setup);

  /* invoke syscall directly */
  retcode = NaCl_tls_get();

  /* reinstall syscallback */
  setup->syscallback = (int32_t) syscall_director;
  zvm_setup(setup);

  return retcode;
}

SYSCALL_MOCK(second_tls_set, 0)

/*
 * get second tls.
 * since we only have single thread use instead of 2nd tls 1st one
 */
static int32_t zrt_second_tls_get(uint32_t *args)
{
  SHOWID;
  return zrt_tls_get(NULL);
}

SYSCALL_MOCK(sem_create, 0)
SYSCALL_MOCK(sem_wait, 0)
SYSCALL_MOCK(sem_post, 0)
SYSCALL_MOCK(sem_get_value, 0)
SYSCALL_MOCK(dyncode_create, 0)
SYSCALL_MOCK(dyncode_modify, 0)
SYSCALL_MOCK(dyncode_delete, 0)
SYSCALL_MOCK(test_infoleak, 0)

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
 * initialize zerovm api, get the user manifest, install syscallback
 * and invoke user code
 */
int main(int argc, char **argv)
{
  setup = zvm_init();
  if(setup == NULL) return ERR_CODE;

  /* todo(d'b): replace it with a good engine. should be done asap */
  setup->channels = (intptr_t) &channels_space_holder;
  if(zvm_set_syscallback((int32_t)syscall_director))
    return ERR_CODE;

  /* call user main() and care about return code */
  return slave_main(argc, argv);
}
