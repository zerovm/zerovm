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

#include "src/loader/sel_ldr.h"

#define HELP_SCREEN /* update command line switches here */\
    "\033[1m\033[37mZeroVM\033[0m lightweight VM manager, build 2013-04-10\n"\
    "Usage: <manifest> [-e#] [-l#] [-v#] [-sFPSQ]\n\n"\
    " <manifest> load settings from manifest file\n"\
    " -e <level> enable the data control. where levels are\n"\
    "    1 - channels, 2 - memory, 3 - both\n"\
    " -l set a new storage limit (in Gb)\n"\
    " -s skip validation\n"\
    " -v <level> verbosity\n"\
    " -F fuzz testing; quit right before starting user app\n"\
    " -P disable channels space preallocation\n"\
    " -Q disable platform qualification (dangerous!)\n"\
    " -S disable signal handling\n"

#define NEXE_PGM_NAME "loner" /* default argv[0] for nexe. deprecated */
#define ZEROVM_PRIORITY 19
#define ZEROVM_IO_LIMIT_UNIT 0x40000000l /* 1gb */
#define ZEROVM_IO_LIMIT 4*ZEROVM_IO_LIMIT_UNIT /* 4gb */
#define IOSTATS_SIZE 0x100
#define LARGEST_NEXE ZEROVM_IO_LIMIT_UNIT

#define MANIFEST_VERSION "09082012"
#define MFT_VERSION "Version"
#define MFT_MEMORY "MemMax"
#define MFT_CHANNEL "Channel"
#define MFT_NEXE "Nexe"
#define MFT_ETAG "NexeEtag"
#define MFT_TIMEOUT "Timeout"
#define MFT_NODE "NodeName"
#define MFT_NAMESERVER "NameServer"
#define MFT_ENVIRONMENT "Environment" /* deprecated */
#define MFT_COMMANDLINE "CommandLine" /* deprecated */

struct SystemManifest
{
  /* zerovm control */
  char *version; /* manifest version */
  int node_id; /* own node id from manifest */

  /* nexe control */
  char *nexe; /* nexe file name */
  char *nexe_etag; /* signature. reserved for a future */
  int32_t timeout; /* time user module allowed to run */
  int32_t user_ret_code; /* nexe return code */

  /* variables for a nexe. deprecated */
  int cmd_line_size; /* command line size for nexe. deprecated */
  char **cmd_line; /* command line for nexe. deprecated */
  char **envp; /* environment variables for user. deprecated */

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
 * set limit for the disk writes (in bytes)
 * return 0 if successfully set
 */
int SetStorageLimit(int64_t a);

/* construct system_manifest object and initialize from manifest */
void SystemManifestCtor(struct NaClApp *nap);

/* close channels, free other resources */
int SystemManifestDtor(struct NaClApp *nap);

/*
 * write report for the proxy and free used resources
 * return 0 if success, otherwise - non 0
 */
int ProxyReport(struct NaClApp *nap);

EXTERN_C_END

#endif
