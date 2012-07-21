/*
 * manifest setup API, zerovm help screen, useful defines
 * todo: move COND_ABORT() to appropriate space
 *
 *  Created on: Nov 30, 2011
 *      Author: d'b
 */
#ifndef SERVICE_RUNTIME_MANIFEST_SETUP_H__
#define SERVICE_RUNTIME_MANIFEST_SETUP_H__ 1

EXTERN_C_BEGIN

#include "api/zvm.h"
#include "src/service_runtime/sel_ldr.h"
#include "stdio.h"

/* todo(d'b): remove useless switches, find a proper header for it */
#define HELP_SCREEN /* update command line switches here */\
    "Usage: ZeroVM [-M manifest_file] [-v d] [-cFSgsQZD]\n\n"\
    " -M <file> load settings from manifest\n"\
    " -v [level] verbosity\n\n"\
    " (testing flags)\n"\
    " -c ignore validator! dangerous!\n"\
    "    repeating this option twice skips validation completely\n"\
    " -F fuzz testing; quit after loading NaCl app\n"\
    " -S disable signal handling.\n"\
    " -g enable gdb debug stub.\n"\
    " -s safely stub out non-validating instructions\n"\
    " -Q disable platform qualification (dangerous!)\n"\
    " -Z use fixed feature x86 CPU mode\n"\
    " -D (switch disabled) enable the UNSTABLE dfa validator\n"
#define NEXE_PGM_NAME "bee" /* argv[0] for nexe */
#define MANIFEST_VERSION "13072012"
#define ZEROVMLOG_NAME "ZeroVM"
#define ZEROVMLOG_OPTIONS (LOG_CONS | LOG_PID | LOG_NDELAY)
#define ZEROVMLOG_PRIORITY LOG_USER

/*
 * todo(d'b): make a decition: leave it here or move to NaClApp
 */
struct SystemManifest
{
  /* zerovm control */
  char *version; /* zerovm version */
  int32_t ret_code; /* zerovm return code */

  /* nexe control */
  char *nexe; /* nexe file name */
  int32_t nexe_max; /* max allowed nexe length */
  char *nexe_etag; /* signature. reserved for a future "short" nexe validation */
  char *etag; /* user output memory digital signature */
  int32_t timeout; /* time user module allowed to run */
//  int32_t user_ret_code; /* nexe return code */
  char user_ret_code[INT32_STRLEN]; /* nexe return code. can be used within signal handler */
  char *user_state; /* nexe exit state (NULL == ok, timeout, terminated by signal) */

  /* variables and limits for a nexe */
  uint32_t heap_ptr; /* pointer to the start of available for user ram */
  uint32_t max_mem; /* memory space available for user program. if 0 = 4gb */
  int cmd_line_size; /* command line size for nexe */
  char **cmd_line; /* command line for nexe */
  char **envp; /* environment variables for user */
  int32_t max_syscalls; /* max allowed *real* system calls, 0 - no limit */
  int32_t cnt_syscalls; /* syscalls counter */
  int32_t syscallback; /* untrusted address callback (see "syscallback.txt") */

  /* channels */
  int32_t channels_count; /* count of channels */
  struct ChannelDesc *channels; /* i/o channels */
};

/*
 * construct system_manifest object and initialize from manifest
 * todo(d'b): everythig about 'report' should be moved to HostManifestCtor()
 */
void SystemManifestCtor(struct NaClApp *nap);

/* deallocate memory, close files, free other resources. put everything in the place */
int SystemManifestDtor(struct NaClApp *nap);

/*
 * write report for the proxy and free used resources
 * return 0 if success, otherwise - non 0
 */
int ProxyReport(struct NaClApp *nap);

/*
 * check number of trap() calls and increment by 1. update
 * system_manifest. return 0 if success, -1 if over limit
 */
int UpdateSyscallsCount(struct NaClApp *nap);

EXTERN_C_END

#endif
