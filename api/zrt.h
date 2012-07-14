/*
 * ZeroVM runtime
 *
 *  Created on: Jul 3, 2012
 *      Author: d'b
 */

#ifndef ZRT_H_
#define ZRT_H_

#include <stdint.h>
#include "zvm.h"

#ifdef USER_SIDE /* disabled for trusted code */

/* enabling */
#define DEBUG 0

#if DEBUG
#define SHOWID do {log_msg((char*)__func__); log_msg("() is called\n");} while(0)
#else
#define SHOWID
#endif

/*
 * user program entry point. old style function prototyping
 * allows to avoid error when main has an empty arguments list
 * note: arguments still can be passed (see "command_line.c")
 *
 * when (and if) "blob engine" will be developed this cheat will
 * be removed
 */
#define main slave_main
int slave_main();

#define JOIN(x,y) x##y
#define ZRT_FUNC(x) JOIN(zrt_, x)

/* mock. replacing real syscall handler */
#define SYSCALL_MOCK(name_wo_zrt_prefix, code) \
static int32_t ZRT_FUNC(name_wo_zrt_prefix)(uint32_t *args)\
{\
  SHOWID;\
  return code;\
}

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

/* entry point for zrt library sample (see "syscall_manager.S" file) */
void syscall_director(void);

#endif /* USER_SIDE */

#endif /* ZRT_H_ */
