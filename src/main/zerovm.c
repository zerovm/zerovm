/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ZeroVM main
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

#include <assert.h>
#include "src/platform/signal.h"
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/platform/qualify.h"
#include "src/main/accounting.h"
#include "src/main/tools.h"
#include "src/channels/preload.h"
#include "src/syscalls/ztrace.h"

#define BADCMDLINE(msg) \
  do { \
    printf(HELP_SCREEN, msg == NULL ? "" : msg, \
        msg == NULL ? "" : "\n", TAG_ENCRYPTION); exit(EINVAL); \
  } while(0)

static int skip_qualification = 0;
static int skip_validation = 0;
static int quit_after_load = 0;

/* log zerovm command line. note: delegates g_string_free to report */
static void CommandLine(int argc, char **argv)
{
  GString *cmd = g_string_new("command =");

  g_string_append_printf(cmd, " %s[%d]", *argv++, getpid());
  while(*argv != NULL)
    g_string_append_printf(cmd, " %s", *argv++);

  SetCmdString(cmd);
  ZLOGS(LOG_DEBUG, "%s", cmd);
}

/* parse given command line, manifest and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *manifest_name = NULL;
  int64_t psize;

  /* construct logger with default verbosity */
  ZLogCtor(LOG_ERROR);
  CommandLine(argc, argv);

  while((opt = getopt(argc, argv, "-PFQst:v:M:T:")) != -1)
  {
    switch(opt)
    {
      case 1:
      case 'M':
        if(manifest_name != NULL)
          BADCMDLINE("2nd manifest encountered");
        manifest_name = optarg;
        break;
      case 's':
        skip_validation = 1;
        ZLOGS(LOG_ERROR, "VALIDATION DISABLED");
        break;
      case 'F':
        quit_after_load = 1;
        break;
      case 't':
        ReportMode(ToInt(optarg));
        break;
      case 'v':
        ZLogDtor();
        ZLogCtor(ToInt(optarg));
        break;
      case 'Q':
        skip_qualification = 1;
        ZLOGS(LOG_ERROR, "PLATFORM QUALIFICATION DISABLED");
        break;
      case 'P':
        ZLOGS(LOG_ERROR, "DISK SPACE PREALLOCATION DISABLED");
        PreloadAllocationDisable();
        break;
      case 'T':
        ZTraceCtor(optarg);
        break;
      default:
        BADCMDLINE(NULL);
        break;
    }
  }

  /* parse manifest file specified in command line */
  if(manifest_name == NULL) BADCMDLINE(NULL);
  nap->manifest = ManifestCtor(manifest_name);

  /* set available nap and manifest fields */
  ZLOGFAIL(nap->manifest->program == NULL, EFAULT, "program not specified");
  psize = GetFileSize(nap->manifest->program);
  ZLOGFAIL(psize < 0, ENOENT, "program open error");
}

static void ValidateProgram(struct NaClApp *nap)
{
  int status = 0; /* 0 = failed, 1 = successful */
  int64_t static_size;
  uint8_t* static_addr;

  assert((nap->static_text_end | MEM_START) > 0);

  /* static and dynamic text address / length */
  static_size = nap->static_text_end -
      NaClSysToUser(MEM_START + NACL_TRAMPOLINE_END);
  static_addr = (uint8_t*)NaClUserToSys(NACL_TRAMPOLINE_END);

  /* validate static and dynamic text */
  if(static_size > 0)
    status = NaClSegmentValidates(static_addr, static_size, nap->initial_entry_pt);

  /* set results */
  SetValidationState(1);
  ZLOGFAIL(status == 0, ENOEXEC, "validation failed");
  SetValidationState(0);
}

int main(int argc, char **argv)
{
  struct NaClApp state = {0}, *nap = &state;
  struct GioMemoryFileSnapshot main_file;

  /* initialize globals and set nap fields to default values */
  ReportCtor();
  NaClAppCtor(nap);
  ParseCommandLine(nap, argc, argv);

  /* We use the signal handler to verify a signal took place. */
  if(skip_qualification == 0) RunSelQualificationTests();
  SignalHandlerInit();

  /* read elf into memory */
  ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&main_file, nap->manifest->program),
      ENOENT, "Cannot open '%s'. %s", nap->manifest->program, strerror(errno));
  ZTrace("[memory snapshot]");

  /* validate program structure (check elf header and segments) */
  ZLOGS(LOG_DEBUG, "Loading %s", nap->manifest->program);
  AppLoadFile((struct Gio *) &main_file, nap);
  ZTrace("[user module loading]");

  /* validate given program (ensure that text segment is safe) */
  ZLOGS(LOG_DEBUG, "Validating %s", nap->manifest->program);
  if(!skip_validation) ValidateProgram(nap);
  ZTrace("[user module validation]");

  /* free snapshot */
  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->manifest->program);
  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  ZTrace("[snapshot deallocation]");

  /* initialize all channels */
  ChannelsCtor(nap->manifest);
  ZLOGS(LOG_DEBUG, "channels constructed");
  ZTrace("[channels mounting]");

  /*
   * allocate user heap. should be the last allocation in raw because
   * after heap allocated there will be no free user memory
   */
  PreallocateUserMemory(nap);
  ZLOGS(LOG_DEBUG, "user memory preallocated");
  ZTrace("[user memory preallocation]");

  /* set user manifest in user space */
  SetSystemData(nap);
  ZLOGS(LOG_DEBUG, "system data set");
  ZTrace("[user manifest construction]");

  /* lock restricted regions in user memory */
  LockRestrictedMemory();
  ZLOGS(LOG_DEBUG, "lock restricted user regions");
  ZTrace("[restricted user regions locking]");

  /* "defense in depth" call */
  ZLOGS(LOG_DEBUG, "Last preparations");
  LastDefenseLine(nap->manifest);

  /* quit if fuzz testing specified */
  if(quit_after_load)
  {
    SetExitState(OK_STATE);
    ReportDtor(0);
  }
  ZTrace("[last preparations]");

  /* switch to the user code flushing all buffers */
  fflush(NULL);
  CreateSession(nap);
  return EFAULT; /* unreachable */
}
