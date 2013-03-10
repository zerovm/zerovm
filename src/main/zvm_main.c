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
#include <errno.h>
#include <glib.h>
#include "src/platform/gio.h"
#include "src/main/nacl_globals.h"
#include "src/platform/nacl_signal.h"
#include "src/main/etag.h"
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h"
#include "src/platform/sel_qualify.h"
#include "src/main/accounting.h"
#include "src/platform/nacl_macros.h"
#include "src/channels/preload.h" /* for PreloadAllocationDisable() */

#define BADCMDLINE(msg) \
  do { \
    printf("\033[2m\033[31m%s%s%s\033[0m", msg == NULL ? "" : msg, \
      msg == NULL ? "" : "\n", HELP_SCREEN); exit(EINVAL); \
  } while(0)

static int skip_qualification = 0;
static int quit_after_load = 0;
static int skip_validator = 0;

/* log zerovm command line */
static void ZVMCommandLine(int argc, char **argv)
{
  char cmd[BIG_ENOUGH_SPACE];
  int offset = 0;
  int i;

  offset += sprintf(cmd, "zerovm command line:");
  for(i = 0; i < argc; ++i)
    offset += g_snprintf(cmd + offset, BIG_ENOUGH_SPACE - offset, " %s", argv[i]);

  ZLOGS(LOG_DEBUG, "%s", cmd);
}

/* parse given command line and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *manifest_name = NULL;

  /* construct zlog with default verbosity */
  ZLogCtor(LOG_ERROR);

  while((opt = getopt(argc, argv, "-PFeQsSv:M:l:")) != -1)
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
        skip_validator = 1;
        ZLOG(LOG_ERROR, "VALIDATION DISABLED by -s");
        break;
      case 'F':
        quit_after_load = 1;
        break;
      case 'e':
        TagEngineCtor();
        nap->user_tag = TagCtor();
        break;
      case 'S':
        /* d'b: disable signals handling */
        SetSignalHandling(0);
        ZLOG(LOG_ERROR, "SIGNAL HANDLING DISABLED by -S");
        break;
      case 'l':
        /* calculate hard limit in Gb and don't allow it less then "big enough" */
        if(SetStorageLimit(ATOI(optarg)) != 0)
          BADCMDLINE("invalid storage limit");
        break;
      case 'v':
        ZLogDtor();
        ZLogCtor(ATOI(optarg));
        break;
      case 'Q':
        skip_qualification = 1;
        ZLOGS(LOG_ERROR, "PLATFORM QUALIFICATION DISABLED by -Q");
        break;
      case 'P':
        PreloadAllocationDisable();
        break;
      default:
        BADCMDLINE(NULL);
        break;
    }
  }

  /* show zerovm command line */
  ZVMCommandLine(argc, argv);

  /* last chance to find command line errors */
  if(manifest_name == NULL) BADCMDLINE(NULL);

  /* parse manifest file specified in command line */
  ZLOGFAIL(ManifestCtor(manifest_name), EFAULT, "Invalid manifest '%s'", manifest_name);

  /* set available nap and manifest fields */
  assert(nap->system_manifest != NULL);
  nap->system_manifest->nexe = GetValueByKey(MFT_NEXE);
  ZLOGFAIL(GetFileSize(nap->system_manifest->nexe) < 0, ENOENT, "nexe open error");
}

/*
 * set validation state according to zvm command line options
 * note: updates nap->validation_state
 */
static void ValidateNexe(struct NaClApp *nap)
{
  char *args[3] = {
#ifdef VALIDATOR_NAME
  sizeof(VALIDATOR_NAME) < 2 ?
      DEFAULT_VALIDATOR_NAME : VALIDATOR_NAME
#else
      DEFAULT_VALIDATOR_NAME
#endif
  };
  GError *error = NULL;
  int exit_status = 0;
  enum ValidationState {
    ValidationOK,
    ValidationFailed,
    NotValidated
  };

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* skip validation? */
  nap->validation_state = NotValidated;
  if(skip_validator != 0) return;

  /* prepare command line and run it */
  args[1] = nap->system_manifest->nexe;

  ZLOGFAIL(g_spawn_sync(NULL, args, NULL,
#ifdef VALIDATOR_NAME
      (sizeof(VALIDATOR_NAME) < 2 ? G_SPAWN_SEARCH_PATH : 0) |
#else
      G_SPAWN_SEARCH_PATH |
#endif
      G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
      NULL, NULL, &exit_status, &error) == 0, EPERM, "cannot start validator");

  /* check the result */
  nap->validation_state = exit_status == 0 ? ValidationOK : ValidationFailed;
  ZLOGFAIL(nap->validation_state != ValidationOK, ENOEXEC, "validation failed");
}

int main(int argc, char **argv)
{
  struct NaClApp state, *nap = &state;
  struct SystemManifest sys_mft;
  struct GioMemoryFileSnapshot main_file;
  GTimer *timer;

  /* zerovm initialization */
  memset(nap, 0, sizeof *nap);
  nap->system_manifest = &sys_mft;
  memset(nap->system_manifest, 0, sizeof *nap->system_manifest);
  gnap = nap;

  ParseCommandLine(nap, argc, argv);

  /* We use the signal handler to verify a signal took place. */
  if(skip_qualification == 0) NaClRunSelQualificationTests();

  NaClSignalHandlerInit();

  /* initialize mem_map and set nap fields to default values */
  ZLOGFAIL(NaClAppCtor(nap) == 0, EFAULT, "Error while constructing app state");

  /* read nexe into memory */
  timer = g_timer_new();
  ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&main_file, nap->system_manifest->nexe),
      ENOENT, "Cannot open '%s'. %s", nap->system_manifest->nexe, strerror(errno));

#define TIMER_REPORT(msg) \
  do {\
    ZLOGS(LOG_DEBUG, msg " took %.3f milliseconds",\
        g_timer_elapsed(timer, NULL) * NACL_MICROS_PER_MILLI);\
    g_timer_start(timer);\
  } while(0)

  TIMER_REPORT(" constructing of memory snapshot");

  /* validate given nexe (ensure that text segment is safe) */
  ValidateNexe(nap);
  TIMER_REPORT(" validating of nacl module");

  /* validate nexe structure (check elf header and segments) */
  ZLOGS(LOG_DEBUG, "Loading nacl file %s", nap->system_manifest->nexe);
  NaClAppLoadFile((struct Gio *) &main_file, nap);

  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->system_manifest->nexe);
  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  TIMER_REPORT(" loading of nacl module");

  /* setup zerovm from manifest */
  SystemManifestCtor(nap);
  TIMER_REPORT(" setting hypervisor from manifest");

  /* "defence in depth" call */
  LastDefenseLine(nap);

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE*) NULL);

  /* start accounting */
  AccountingCtor(nap);

  /* quit if fuzz testing specified */
  if(quit_after_load)
  {
    SetExitState(OK_STATE);
    NaClExit(0);
  }
  TIMER_REPORT(" last preparations");

  /* set user code trap() exit location and switch to the user code */
  if(setjmp(user_exit) == 0)
    ZLOGFAIL(!NaClCreateMainThread(nap), EFAULT, "switching to nexe failed");
  SetExitState(OK_STATE);
  TIMER_REPORT(" nacl module session");

  /* zerovm exit with finalization, report and stuff */
  NaClExit(0);

  /* Unreachable, but having the return prevents a compiler error. */
  return -1;
}
