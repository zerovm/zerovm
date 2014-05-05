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
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/syscalls/switch_to_app.h"

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
static char *ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *mft = NULL;

  CommandLine(argc, argv);

  while((opt = getopt(argc, argv, "-PFQst:v:M:T:")) != -1)
  {
    switch(opt)
    {
      case 1:
      case 'M':
        if(mft != NULL)
          BADCMDLINE("2nd manifest encountered");
        mft = optarg;
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

  if(mft == NULL) BADCMDLINE(NULL);
  return g_strdup(mft);
}

int main(int argc, char **argv)
{
  struct NaClApp state = {0};

  /* set logger in case it will be used during initialization */
  ZLogCtor(LOG_ERROR);
  SessionCtor(&state, ParseCommandLine(&state, argc, argv));

  /* quit if fuzz testing specified */
  if(CommandPtr()->quit_after_load)
    SessionDtor(0, OK_STATE);

  /* switch to the user code flushing all buffers */
  fflush(NULL);
  ZLOGS(LOG_DEBUG, "SESSION %s STARTED", state.manifest->node);
  ContextSwitch(nacl_user);

  return EFAULT; /* unreachable */
}
