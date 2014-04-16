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
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
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

/* log zerovm command line. note: delegates g_string_free to report */
static void CommandLine(int argc, char **argv)
{
  GString *cmd = g_string_new("command =");

  g_string_append_printf(cmd, " %s[%d]", *argv++, getpid());
  while(*argv != NULL)
    g_string_append_printf(cmd, " %s", *argv++);

  ZLOGS(LOG_DEBUG, "%s", cmd->str);
  ReportSetupPtr()->cmd = g_strdup(cmd->str);
  g_string_free(cmd, TRUE);
}

/* parse given command line, manifest and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *manifest = NULL;

  /* construct logger with default verbosity */
  ZLogCtor(LOG_ERROR);
  CommandLine(argc, argv);

  while((opt = getopt(argc, argv, "-PFQst:v:M:T:")) != -1)
  {
    switch(opt)
    {
      case 1:
      case 'M':
        if(manifest != NULL)
          BADCMDLINE("2nd manifest encountered");
        manifest = optarg;
        break;
      case 's':
        CommandPtr()->skip_validation = 1;
        ZLOGS(LOG_ERROR, "VALIDATION DISABLED");
        break;
      case 'F':
        CommandPtr()->quit_after_load = 1;
        break;
      case 't':
        CommandPtr()->report_mode = ToInt(optarg);
        break;
      case 'v':
        CommandPtr()->zlog_verbosity = ToInt(optarg);
        ZLogCtor(ToInt(optarg));
        break;
      case 'Q':
        CommandPtr()->skip_qualification = 1;
        ZLOGS(LOG_ERROR, "PLATFORM QUALIFICATION DISABLED");
        break;
      case 'P':
        CommandPtr()->preload_allocation_disable = 1;
        ZLOGS(LOG_ERROR, "DISK SPACE PREALLOCATION DISABLED");
        break;
      case 'T':
        CommandPtr()->ztrace_name = g_strdup(optarg);
        break;
      default:
        BADCMDLINE(NULL);
        break;
    }
  }

  /* initialize session */
  if(manifest == NULL) BADCMDLINE(NULL);
  nap->manifest = ManifestCtor(manifest);
}

static void ValidateProgram(struct NaClApp *nap)
{
  int status = 0; /* 0 = failed, 1 = successful */
  int64_t static_size;
  uint8_t* static_addr;

  assert(nap->static_text_end > 0);

  /* static and dynamic text address / length */
  static_size = nap->static_text_end -
      NaClSysToUser(MEM_START + NACL_TRAMPOLINE_END);
  static_addr = (uint8_t*)NaClUserToSys(NACL_TRAMPOLINE_END);

  /* validate static and dynamic text */
  if(static_size > 0)
    status = NaClSegmentValidates(static_addr, static_size, nap->initial_entry_pt);

  /* set results */
  ReportSetupPtr()->validation_state = 1;
  ZLOGFAIL(status == 0, ENOEXEC, "validation failed");
  ReportSetupPtr()->validation_state = 0;
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
  if(CommandPtr()->skip_qualification == 0)
    RunSelQualificationTests();
  SignalHandlerInit();

  /* read elf into memory */
  ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&main_file, nap->manifest->program),
      ENOENT, "Cannot open '%s'. %s", nap->manifest->program, strerror(errno));
  ZTrace("[memory snapshot]");

  /* initialize all channels */
  /* TODO(d'b): should be done *after* validation */
  ChannelsCtor(nap->manifest);
  ZLOGS(LOG_DEBUG, "channels constructed");
  ZTrace("[channels mounting]");

  /* validate program structure (check elf header and segments) */
  ZLOGS(LOG_DEBUG, "Loading %s", nap->manifest->program);
  AppLoadFile((struct Gio *) &main_file, nap);
  ZTrace("[user module loading]");

  /* validate given program (ensure that text segment is safe) */
  ZLOGS(LOG_DEBUG, "Validating %s", nap->manifest->program);
  if(CommandPtr()->skip_validation == 0)
    ValidateProgram(nap);
  ZTrace("[user module validation]");

  /* free snapshot */
  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->manifest->program);
  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  ZTrace("[snapshot deallocation]");

  /* lock restricted regions in user memory */
  LockRestrictedMemory();
  ZLOGS(LOG_DEBUG, "lock restricted user regions");
  ZTrace("[restricted user regions locking]");

  /* "defense in depth" call */
  ZLOGS(LOG_DEBUG, "Last preparations");
  LastDefenseLine(nap->manifest);

  /* quit if fuzz testing specified */
  if(CommandPtr()->quit_after_load)
  {
    ReportSetupPtr()->zvm_state = g_strdup(OK_STATE);
    ReportDtor(0);
  }
  ZTrace("[last preparations]");

  /* switch to the user code flushing all buffers */
  fflush(NULL);
  CreateSession(nap);
  return EFAULT; /* unreachable */
}
