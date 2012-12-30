/*
 * manifest setup API, zerovm help screen, useful defines
 *
 * Copyright (c) 2012, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SERVICE_RUNTIME_MANIFEST_SETUP_H__
#define SERVICE_RUNTIME_MANIFEST_SETUP_H__ 1

EXTERN_C_BEGIN

#include "api/zvm.h"
#include "src/loader/sel_ldr.h"
#include "stdio.h"

#define HELP_SCREEN /* update command line switches here */\
    "\033[1mZeroVM\033[0m lightweight VM manager, build 2012-12-29\n"\
    "Usage: <-M manifest_file> [-v #] [-l #] [-FSseQ]\n\n"\
    " -M <file> load settings from manifest\n"\
    " -e enable the data oversight (etag)\n"\
    " -l set a new storage limit (in Gb)\n"\
    " -s skip validation\n\n"\
    " (testing flags)\n"\
    " -v [level] verbosity\n"\
    " -F fuzz testing; quit after loading NaCl app\n"\
    " -S disable signal handling\n"\
    " -Q disable platform qualification (dangerous!)\n"

/* todo(d'b): order and move it to zerovm setup or gather all zvm settings here {{ */
#define NEXE_PGM_NAME "loner" /* default argv[0] for nexe */
#define MANIFEST_VERSION "09082012"
#define ZEROVM_PRIORITY 19
#define ZEROVM_IO_LIMIT_UNIT 0x40000000l
#define ZEROVM_IO_LIMIT 4*ZEROVM_IO_LIMIT_UNIT
#define NEW_ROOT "/"
#define VALIDATOR_NAME "ncval"
#define IOSTATS_SIZE 0X100
/* }} */

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
  char *nexe_etag; /* signature. reserved for a future "short" nexe validation */
  unsigned char *etag; /* channels and user memory digital signature */
  int32_t timeout; /* time user module allowed to run */
  int32_t user_ret_code; /* nexe return code */

  /* variables and limits for a nexe */
  int cmd_line_size; /* command line size for nexe */
  char **cmd_line; /* command line for nexe */
  char **envp; /* environment variables for user */
  int32_t syscallback; /* untrusted address callback (see "syscallback.txt") */

  /* channels */
  int32_t channels_count; /* count of channels */
  struct ChannelDesc *channels; /* i/o channels */
};

/*
 * "defense in depth". the last frontier of defense.
 * zerovm limits itself as much as possible
 */
void LastDefenseLine();

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

EXTERN_C_END

#endif
