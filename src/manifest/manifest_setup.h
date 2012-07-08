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
#define MANIFEST_VERSION "11062012"

struct HostManifest
{
  int32_t ret_code; /* zerovm return code */
  char *etag; /* user output memory digital signature */
  int32_t user_ret_code; /* nexe return code */
  char *content_type; /* custom user attribute */
  char *x_object_meta_tag; /* custom user attribute */
};

struct SystemManifest
{
  char *version; /* zerovm version */
  char *log; /* zerovm log file name */
  FILE *report;  /* report to proxy file descriptor */
  char *nexe; /* nexe file name */
  int cmd_line_size; /* command line size for nexe */
  char **cmd_line; /* command line for nexe */
  int32_t nexe_max; /* max allowed nexe length */
  char *nexe_etag; /* digital signature. reserved for a future "short" nexe validation */
  int32_t timeout; /* time user module allowed to run */

  COMMON_PART /* shared with struct UserManifest */
};

/*
 * construct system_manifest object and initialize from manifest
 * todo(d'b): everythig about 'report' should be moved to HostManifestCtor()
 */
void SystemManifestCtor(struct NaClApp *nap);

/* construct host_manifest and initialize from manifest */
void HostManifestCtor(struct NaClApp *nap);

/* deallocate memory, close files, free other resources. put everything in the place */
int SystemManifestDtor(struct NaClApp *nap);

/*
 * write report for the proxy and free used resources
 * return 0 if success, otherwise - non 0
 */
int HostManifestDtor(struct NaClApp *nap);

/*
 * check number of trap() calls and increment by 1. update
 * system_manifest. return 0 if success, -1 if over limit
 */
int UpdateSyscallsCount(struct NaClApp *nap);

/*
 * check if given counter not overrun the limit and increment by 1.
 * update system_manifest. return 0 if success, -1 if over limit
 */
int UpdateIOCounter(struct NaClApp *nap, enum ChannelType ch, enum IOCounters cntr);

EXTERN_C_END

#endif
