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
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/main/zlog.h"
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

/* parse given command line, manifest and initialize session settings */
static char *ParseCommandLine(int argc, char **argv)
{
  int opt;
  char *mft = NULL;

  CommandLine(argc, argv);

  while((opt = getopt(argc, argv, "-PFQs::t:v:M:T:")) != -1)
  {
    switch(opt)
    {
      case 1:
      case 'M':
        if(mft != NULL)
          BADCMDLINE("2nd manifest encountered");
        mft = g_strdup(optarg);
        break;
      case 's':
        /*
         * patch to make prevalidation with uboot. skip validation 1st time
         * for uboot itself and skip validation 2nd time for loaded elf.
         * each time skip_validation will be decreased by 1. when it will
         * reach 1 skip_validation value will be set to 0 and all other
         * validations will trigger real validation process
         */
        opt = optarg ? ToInt(optarg) : 2;
        if(opt < 0 || opt > 2)
          BADCMDLINE("invalid validation value");
        CommandPtr()->skip_validation = opt > 1 ? 3 : opt;
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
  return mft;
}

int main(int argc, char **argv)
{
  char *mft;

  /* set logger in case it will be used during initialization */
  ZLogCtor(LOG_ERROR);
  mft = ParseCommandLine(argc, argv);
  SessionCtor(mft);
  g_free(mft);

  /* switch to the user code */
  ContextSwitch(nacl_user);

  return EFAULT; /* unreachable */
}
