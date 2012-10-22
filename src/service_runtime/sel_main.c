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
  int verbosity = 0;

  /* set defaults */
  nap->skip_qualification = 0;
  nap->quit_after_load = 0;
  nap->handle_signals = 1;
  nap->storage_limit = ZEROVM_IO_LIMIT;

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
        verbosity = ATOI(optarg);
        verbosity = MAX(verbosity, 0);
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

  /* construct zlog */
  ZLogCtor(verbosity);

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
  syscallback = 0;
}

/* set zerovm state by given message */
/* todo(): move it to "tools.h", zvm_state should not be updated directly */
static void SetZVMState(struct NaClApp *nap, const char *msg)
{
  snprintf(nap->zvm_state, SIGNAL_STRLEN, "%s", msg);
  nap->zvm_state[SIGNAL_STRLEN] = '\0';
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

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* skip validation? */
  nap->validation_state = NotValidated;
  if(nap->skip_validator != 0) return;

  /* check nexe */
  ZLOGFAIL(GetFileSize(nap->system_manifest->nexe) < 0, ENOENT, "nexe open error");

  /* prepare command line and run it */
  args[1] = nap->system_manifest->nexe;
  ZLOGFAIL(g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH |
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
  NaClErrorCode errcode = LOAD_INTERNAL;
  struct GioFile gout;
  struct GioMemoryFileSnapshot main_file;
  struct NaClPerfCounter time_all_main;

  /* d'b: initial settings */
  /* todo(d'b): move to inline function and explicitly set all fields */
  memset(nap, 0, sizeof *nap);
  nap->system_manifest = &sys_mft;
  memset(nap->system_manifest, 0, sizeof *nap->system_manifest);
  gnap = nap;
  ZLogCtor(0); /* make log working */
  SetZVMState(nap, "nexe didn't start");
  NaClSignalHandlerInit();

  NaClAllModulesInit();
  NaClPerfCounterCtor(&time_all_main, "SelMain");

  ParseCommandLine(nap, argc, argv);

  /* todo(d'b): ### does zerovm needs it? */
  ZLOGFAIL(!GioFileRefCtor(&gout, stdout), EIO,
      "Could not create general standard output channel");

  /* validate given nexe and run/fail/exit */
  ValidateNexe(nap);

  /* the dyn_array constructor 1st call */
  ZLOGFAIL(NaClAppCtor(nap) == 0, EFAULT, "Error while constructing app state");
  errcode = LOAD_OK;

  /* We use the signal handler to verify a signal took place. */
  if(nap->skip_qualification == 0)
  {
    NaClErrorCode pq_error = NaClRunSelQualificationTests();
    if(LOAD_OK != pq_error)
    {
      errcode = pq_error;
      nap->module_load_status = pq_error;
      ZLOG(LOG_ERROR, "Error while loading \"%s\": %s",
          NULL != nap->system_manifest->nexe ?
              nap->system_manifest->nexe : "(no file, to-be-supplied-via-RPC)",
          NaClErrorString(errcode));
    }
  }

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
  if(LOAD_OK == errcode)
  {
    ZLOGS(LOG_DEBUG, "Loading nacl file %s", nap->system_manifest->nexe);
    errcode = NaClAppLoadFile((struct Gio *) &main_file, nap);
    if (LOAD_OK != errcode)
    {
      ZLOG(LOG_ERROR, "Error while loading '%s': %s", nap->system_manifest->nexe,
          NaClErrorString(errcode));
      ZLOG(LOG_ERROR, "Using the wrong type of nexe (nacl-x86-32 on an x86-64)");
      ZLOG(LOG_ERROR, "or a corrupt nexe file may be responsible for this error");
    }

    PERF_CNT("AppLoadEnd");
    nap->module_load_status = errcode;
  }

  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->system_manifest->nexe);

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if(nap->quit_after_load) NaClExit(0);

  /* setup zerovm from manifest */
  SystemManifestCtor(nap); /* needs dyn_array initialized */

  /* error reporting done; can quit now if there was an error earlier */
  /* remove it with nacl error */
  ZLOGFAIL(LOAD_OK != errcode, EFAULT, "Not running app, errcode is %d", errcode);

  /*
   * "defence in depth" part
   * todo(): find a proper place for this call
   */
  LastDefenseLine(nap);

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE *) NULL);

  /* start accounting */
  AccountingCtor(nap);

  /* set user code trap() exit location and switch to the user code */
  PERF_CNT("CreateMainThread");
  if(setjmp(user_exit) == 0)
    ZLOGFAIL(!NaClCreateMainThread(nap), EFAULT, "switching to nexe failed");
  PERF_CNT("WaitForMainThread");
  PERF_CNT("SelMainEnd");

  /* stop accounting and harvest info */
  AccountingDtor(nap);

  /* report to host. call destructors. exit */
  ZLogDtor();
  NaClExit(0);

  /* Unreachable, but having the return prevents a compiler error. */
  return -1;
}
