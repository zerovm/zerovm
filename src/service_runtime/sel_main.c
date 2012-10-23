/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ZeroVM main
 */

#include <assert.h>
#include <errno.h>
#include <glib.h>
#include "src/gio/gio.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/etag.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/sel_qualify.h"
#include "src/service_runtime/accounting.h"

/* log zerovm command line */
static void ZVMCommandLine(int argc, char **argv)
{
  char cmd[BIG_ENOUGH_SPACE];
  int offset = 0;
  int i;

  offset += sprintf(cmd, "zerovm command line:");
  for(i = 0; i < argc; ++i)
    offset += snprintf(cmd + offset, BIG_ENOUGH_SPACE - offset, " %s", argv[i]);

  ZLOGS(LOG_DEBUG, "%s", cmd);
}

/* parse given command line and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *manifest_name = NULL;
  ZENTER;

  /* set defaults */
  nap->skip_qualification = 0;
  nap->quit_after_load = 0;
  nap->handle_signals = 1;
  nap->storage_limit = ZEROVM_IO_LIMIT;

  /* construct zlog with default verbosity */
  ZLogCtor(LOG_ERROR);

  /* todo(d'b): revise switches and rename them */
  while((opt = getopt(argc, argv, "+FeQsSv:M:l:")) != -1)
  {
    switch(opt)
    {
      case 'M':
        manifest_name = optarg;
        break;
      case 's':
        nap->skip_validator = 1;
        ZLOG(LOG_ERROR, "validation disabled by -s");
        break;
      case 'F':
        nap->quit_after_load = 1;
        break;
      case 'e':
        TagEngineCtor();
        break;
      case 'S':
        /* d'b: disable signals handling */
        nap->handle_signals = 0;
        break;
      case 'l':
        /* calculate hard limit in Gb and don't allow it less then "big enough" */
        nap->storage_limit = ATOI(optarg) * ZEROVM_IO_LIMIT_UNIT;
        ZLOGFAIL(nap->storage_limit < ZEROVM_IO_LIMIT_UNIT, EFAULT,
            "invalid storage limit: %d", nap->storage_limit);
        break;
      case 'v':
        ZLogDtor();
        ZLogCtor(ATOI(optarg));
        break;
      case 'Q':
        nap->skip_qualification = 1;
        ZLOG(LOG_ERROR, "PLATFORM QUALIFICATION DISABLED BY -Q - "
            "Native Client's sandbox will be unreliable!");
        break;
      default:
        ZLOG(LOG_ERROR, "ERROR: unknown option: [%c]", opt);
        puts(HELP_SCREEN);
        exit(EINVAL);
        break;
    }
  }

  /* show zerovm command line */
  ZVMCommandLine(argc, argv);

  /* parse manifest file specified in cmdline */
  if(manifest_name == NULL)
  {
    puts(HELP_SCREEN);
    exit(EINVAL);
  }
  ZLOGFAIL(ManifestCtor(manifest_name), EFAULT, "Invalid manifest '%s'", manifest_name);

  /* set available nap and manifest fields */
  assert(nap->system_manifest != NULL);
  nap->system_manifest->nexe = GetValueByKey("Nexe");
  ZLOGFAIL(GetFileSize(nap->system_manifest->nexe) < 0, ENOENT, "nexe open error");
  syscallback = 0;
  ZLEAVE;
}

/*
 * set validation state according to zvm command line options
 * note: updates nap->validation_state
 */
static void ValidateNexe(struct NaClApp *nap)
{
  char *args[3] = {VALIDATOR_NAME, NULL, NULL};
  GError *error = NULL;
  int exit_status = 0;
  enum ValidationState {
    NotValidated,
    ValidationOK,
    ValidationFailed
  };
  ZENTER;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* skip validation? */
  nap->validation_state = NotValidated;
  if(nap->skip_validator != 0) return;

  /* prepare command line and run it */
  args[1] = nap->system_manifest->nexe;
  ZLOGFAIL(g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH |
      G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
      NULL, NULL, &exit_status, &error) == 0, EPERM, "cannot start validator");

  /* check the result */
  nap->validation_state = exit_status == 0 ? ValidationOK : ValidationFailed;
  ZLOGFAIL(nap->validation_state != ValidationOK, ENOEXEC, "validation failed");
  ZLEAVE;
}

int main(int argc, char **argv)
{
  struct NaClApp state, *nap = &state;
  struct SystemManifest sys_mft;
  struct GioMemoryFileSnapshot main_file;
  struct NaClPerfCounter time_all_main;

  /* d'b: initial settings */
  /* todo(d'b): move to inline function and explicitly set all fields */
  memset(nap, 0, sizeof *nap);
  nap->system_manifest = &sys_mft;
  memset(nap->system_manifest, 0, sizeof *nap->system_manifest);
  gnap = nap;

  ParseCommandLine(nap, argc, argv);
  NaClSignalHandlerInit();
  NaClAllModulesInit();
  NaClPerfCounterCtor(&time_all_main, "SelMain");


  /* validate given nexe and run/fail/exit */
  ValidateNexe(nap);

  /* the dyn_array constructor 1st call */
  ZLOGFAIL(NaClAppCtor(nap) == 0, EFAULT, "Error while constructing app state");

  /* We use the signal handler to verify a signal took place. */
  if(nap->skip_qualification == 0) NaClRunSelQualificationTests();

  /* Remove the signal handler if we are not using it. */
  if(nap->handle_signals == 0)
  {
    NaClSignalHandlerFini();
    NaClSignalAssertNoHandlers(); /* Sanity check. */
  }

#define PERF_CNT(str) \
  NaClPerfCounterMark(&time_all_main, str);\
  NaClPerfCounterIntervalLast(&time_all_main);

  ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&main_file, nap->system_manifest->nexe),
      ENOENT, "Cannot open '%s'. %s", nap->system_manifest->nexe, strerror(errno));

  PERF_CNT("SnapshotNaclFile");

  /* validate untrusted code (nexe) */
  ZLOGS(LOG_DEBUG, "Loading nacl file %s", nap->system_manifest->nexe);
  NaClAppLoadFile((struct Gio *) &main_file, nap);
  PERF_CNT("AppLoadEnd");

  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->system_manifest->nexe);

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if(nap->quit_after_load) NaClExit(0);

  /* setup zerovm from manifest */
  SystemManifestCtor(nap); /* needs dyn_array initialized */

  /* "defence in depth" call */
  LastDefenseLine(nap);

  /* start accounting */
  AccountingCtor(nap);

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE*) NULL);

  /* set user code trap() exit location and switch to the user code */
  PERF_CNT("CreateMainThread");
  if(setjmp(user_exit) == 0)
    ZLOGFAIL(!NaClCreateMainThread(nap), EFAULT, "switching to nexe failed");
  SetExitState(OK_STATE);
  PERF_CNT("WaitForMainThread");
  PERF_CNT("SelMainEnd");

  /* zerovm exit with finalization, report and stuff */
  NaClExit(0);

  /* Unreachable, but having the return prevents a compiler error. */
  return -1;
}
