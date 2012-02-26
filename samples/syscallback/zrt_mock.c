/*
 * zrt_mock.c
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
#include "api/zvm.h"

#include <unistd.h> /* only for tests */

/* entry point for zrt library sample (see "syscall_manager.S" file) */
void syscall_director(void);

/* need to be here for selective syscallback */
struct SetupList setup;

/* debug purposes only */
#define DEBUG 1
#define LOGFIX /* temporary fix until zrt library will be finished */
#if DEBUG
#define SHOWID do {log_msg("\n"); log_msg((char*)__func__); log_msg("() is called\n");} while(0)
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
#define NaCl_mmap(start, length, prot, flags, d, offp) ((int32_t (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)) \
    (NACL_sys_mmap * 0x20 + 0x10000))(start, length, prot, flags, d, offp)

/* tls_get() -- nacl syscall via trampoline */
#define NaCl_tls_get() ((int32_t (*)()) \
    (NACL_sys_second_tls_get * 0x20 + 0x10000))()

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
  SHOWID; return 0; /* ### use "not implemented" error code from zerovm */
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

/* mock. should be implemented */
int32_t zrt_dup(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_dup2(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_open(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_close(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_read(uint32_t *args)
{
  SHOWID; return 0;
}

/* example how to implement zrt syscall */
int32_t zrt_write(uint32_t *args)
{
  SHOWID;
  log_msg("the given buffer = [");
  log_msg((char*)args[1]);
  log_msg("]\n");

  /*
   * to prevent deadloop the return code must be or negative
   * (if fail) or positive (amount of written bytes)
   */
  return args[2];
}

/* mock. should be implemented */
int32_t zrt_lseek(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_ioctl(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_stat(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_fstat(uint32_t *args)
{
  SHOWID;
  return -1; /* return fail */
}

/* mock. should be implemented */
int32_t zrt_chmod(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_sysbrk(uint32_t *args)
{
  SHOWID; return 0;
}

/* example of "selective syscallback". this syscall handed back to zerovm */
int32_t zrt_mmap(uint32_t *args)
{
  int32_t retcode;
  SHOWID;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);
  log_msg("syscallback uninstalled\n");

  /* invoke syscall directly */
  retcode = NaCl_mmap(args[0], args[1], args[2], args[3], args[4], args[5]);
  log_msg("syscallback retranslated to zerovm\n");

  /* reinstall syscallback */
  log_msg("...returned to zrt_mmap()\n");
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);
  log_msg("syscallback reinstalled\n");

  return retcode;
}

/* mock. should be implemented */
int32_t zrt_munmap(uint32_t *args)
{
  SHOWID; return 0;
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
  SHOWID; zvm_exit(args[0]);

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

/* mock. should be implemented */
int32_t zrt_gettimeofday(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_clock(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
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

/* example of "selective syscallback". this syscall handed back to zerovm */
int32_t zrt_tls_get(uint32_t *args)
{
  int32_t retcode;
  SHOWID;

  /* uninstall syscallback */
  setup.syscallback = 0;
  zvm_setup(&setup);
  log_msg("syscallback uninstalled\n");

  /* invoke syscall directly */
  retcode = NaCl_tls_get();
  log_msg("syscallback retranslated to zerovm\n");

  /* reinstall syscallback */
  log_msg("...returned\n");
  setup.syscallback = (int32_t) syscall_director;
  zvm_setup(&setup);
  log_msg("syscallback reinstalled\n");

  return retcode;
}

/* mock. should be implemented */
int32_t zrt_second_tls_set(uint32_t *args)
{
  SHOWID; return 0;
}

/* mock. should be implemented */
int32_t zrt_second_tls_get(uint32_t *args)
{
  SHOWID; return 0;
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

int main()
{
  int retcode = ERR_CODE;
  char hello[] = "hello";

  /*
   * install syscallback function
   * note: it will also get user manifest
   */
  setup.syscallback = (int32_t) syscall_director;
  retcode = zvm_setup(&setup);
  if(retcode) return retcode;

#ifdef LOGFIX
  /* set up the log */
  retcode = log_set(&setup);
  if(retcode) return retcode;
#endif

  /* say hello */
  log_msg("hello, syscallback program started\n");
  log_msg("syscallback installed\n");

  /* example how to invoke nacl syscall directly */
  NaCl_write(1, (int32_t)hello, 6);

  /* call some c functions */
  printf("Where have all the flowers gone?\n");
  fflush(NULL);
  printf("Long time passing\n");
  printf("var1 = %d, var2 = %d, var3 = %d\n", 1, 2, 3);
  fopen("something", "r");
  close(1);

  /* try to call invalid syscall */
  NaCl_invalid();

  return retcode;
}
