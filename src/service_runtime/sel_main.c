/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ZeroVM main
 */

#include <assert.h>
#include <glib.h>
#include "src/gio/gio.h"
#include "src/fault_injection/fault_injection.h"
#include "src/perf_counter/nacl_perf_counter.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/etag.h"
#include "src/manifest/manifest_parser.h" /* d'b. todo: move to initializer */
#include "src/manifest/manifest_setup.h" /* d'b. todo: move to initializer */
#include "src/service_runtime/sel_qualify.h"
#include "src/service_runtime/accounting.h"

/* initialize syslog to put ZeroVm log messages */
static void ZeroVMLogCtor()
{
  /*
   * are here to avoid redefinition LOG_* used in "nacl_log.h"
   * todo(d'b): remove this ugliness
   */
#define LOG_PID   0x01  /* log the pid with each message */
#define LOG_CONS  0x02  /* log on the console if errors in sending */
#define LOG_NDELAY  0x08  /* don't delay open */
#define LOG_USER  (1<<3)  /* random user-level messages */
  extern void openlog (const char *ident, int option, int facility);

  openlog(ZEROVMLOG_NAME, ZEROVMLOG_OPTIONS, ZEROVMLOG_PRIORITY);
}

/* close the log. ### optional? */
static void ZeroVMLogDtor()
{
  extern void closelog (void); /* to avoid redefinition LOG_* */
  closelog();
}

/* parse given command line and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  char cmd[BIG_ENOUGH_SPACE];
  int opt;
  int i;
  char *manifest_name = NULL;

  /* set defaults */
  nap->verbosity = NaClLogGetVerbosity();
  nap->skip_qualification = 0;
  nap->fuzzing_quit_after_load = 0;
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
        NaClLog(LOG_WARNING, "validation disabled by -s\n");
        break;
      case 'F':
        nap->fuzzing_quit_after_load = 1;
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
        FailIf(nap->storage_limit < ZEROVM_IO_LIMIT_UNIT,
            "invalid storage limit: %d", nap->storage_limit);
        break;
      case 'v':
        NaClLogSetVerbosity(ATOI(optarg));
        nap->verbosity = NaClLogGetVerbosity();
        break;
      case 'Q':
        nap->skip_qualification = 1;
        NaClLog(LOG_WARNING, "PLATFORM QUALIFICATION DISABLED BY -Q - "
                "Native Client's sandbox will be unreliable!\n");
        break;
      default:
        NaClLog(LOG_ERROR, "ERROR: unknown option: [%c]\n\n", opt);
        puts(HELP_SCREEN);
        exit(1);
        break;
    }
  }

  /* show zerovm command line */
  strcpy(cmd, "zerovm command line:");
  for(i = 0; i < argc; ++i)
  {
    strncat(cmd, " ", BIG_ENOUGH_SPACE);
    strncat(cmd, argv[i], BIG_ENOUGH_SPACE);
  }
  NaClLog(LOG_NOTE, "%s", cmd);

  /* parse manifest file specified in cmdline */
  if(manifest_name == NULL)
  {
    puts(HELP_SCREEN);
    exit(1);
  }
  COND_ABORT(ManifestCtor(manifest_name), "Invalid manifest file");

  /* set available nap and manifest fields */
  assert(nap->system_manifest != NULL);
  nap->user_side_flag = 0; /* we are in the trusted code */
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

  /* prepare command line and run it */
  args[1] = nap->system_manifest->nexe;
  COND_ABORT(g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH |
      G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL,
      NULL, NULL, &exit_status, &error) == 0, "cannot start validator");

  /* check the result */
  nap->validation_state = exit_status == 0 ? ValidationOK : ValidationFailed;
  COND_ABORT(nap->validation_state != ValidationOK, "validation failed");
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
  /* todo(d'b): move to inline function and implicitly set all fields */
  memset(nap, 0, sizeof *nap);
  nap->trusted_code = 1;
  nap->system_manifest = &sys_mft;
  memset(nap->system_manifest, 0, sizeof *nap->system_manifest);
  gnap = nap;
  SetZVMState(nap, "nexe didn't start");
  ZeroVMLogCtor();
  NaClSignalHandlerInit();

  NaClAllModulesInit();
  NaClPerfCounterCtor(&time_all_main, "SelMain");
  COND_ABORT(!GioFileRefCtor(&gout, stdout),
             "Could not create general standard output channel");

  ParseCommandLine(nap, argc, argv);

  /* validate given nexe and run/fail/exit */
  ValidateNexe(nap);

  /* todo(d'b): remove it after validator will be removed from the project */
  NaClLogGetGio();

  /* the dyn_array constructor 1st call */
  COND_ABORT(NaClAppCtor(nap) == 0, "Error while constructing app state");
  errcode = LOAD_OK;

  /* We use the signal handler to verify a signal took place. */
  if(nap->skip_qualification == 0)
  {
    NaClErrorCode pq_error = NACL_FI_VAL("pq", NaClErrorCode, NaClRunSelQualificationTests());
    if(LOAD_OK != pq_error)
    {
      errcode = pq_error;
      nap->module_load_status = pq_error;
      NaClLog(LOG_ERROR, "Error while loading \"%s\": %s\n",
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

#define PERF_CNT(str)\
  NaClPerfCounterMark(&time_all_main, str);\
  NaClPerfCounterIntervalLast(&time_all_main);

  if(0 == GioMemoryFileSnapshotCtor(&main_file, nap->system_manifest->nexe))
  {
    NaClLog(LOG_ERROR, "%s", strerror(errno));
    NaClLog(LOG_FATAL, "Cannot open \"%s\".\n", nap->system_manifest->nexe);
  }
  PERF_CNT("SnapshotNaclFile");

  /* validate untrusted code (nexe) */
  if(LOAD_OK == errcode)
  {
    NaClLog(2, "Loading nacl file %s (non-RPC)\n", nap->system_manifest->nexe);
    errcode = NaClAppLoadFile((struct Gio *) &main_file, nap);
    if (LOAD_OK != errcode)
    {
      NaClLog(LOG_ERROR, "Error while loading \"%s\": %s\n",
              nap->system_manifest->nexe, NaClErrorString(errcode));
      NaClLog(LOG_ERROR, ("Using the wrong type of nexe (nacl-x86-32"
          " on an x86-64 or vice versa)\nor a corrupt nexe file may be"
          " responsible for this error.\n"));
    }

    PERF_CNT("AppLoadEnd");
    nap->module_load_status = errcode;
  }

  if(-1 == (*((struct Gio *) &main_file)->vtbl->Close)((struct Gio *) &main_file))
    NaClLog(LOG_ERROR, "Error while closing \"%s\".\n", nap->system_manifest->nexe);

  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);
  if(nap->fuzzing_quit_after_load) NaClExit(0);

  /*
   * construct system and host manifests
   * todo(d'b): move it to "src/platform/platform_init.c" chain. problems to solve:
   * - channels construction needs initialized nacl descriptors (it is dyn_array)
   * - "memory chunk" needs initialized memory manager (user stack, text, data e.t.c)
   */
  SystemManifestCtor(nap); /* needs dyn_array initialized */

  /* error reporting done; can quit now if there was an error earlier */
  if(LOAD_OK != errcode)
    NaClLog(LOG_FATAL, "Not running app code since errcode is %s (%d)\n",
            NaClErrorString(errcode), errcode);

  PERF_CNT("CreateMainThread");

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE *) NULL);

  /*
   * "defence in depth" part
   * todo(): find a proper place for this call
   */
  LastDefenseLine(nap);

  /* start accounting */
  AccountingCtor(nap);

  /* set user code trap() exit location */
  if(setjmp(user_exit) == 0)
  {
    /* pass control to the user code */
    if(!NaClCreateMainThread(nap))
      NaClLog(LOG_FATAL, "creating main thread failed\n");
  }
  PERF_CNT("WaitForMainThread");
  PERF_CNT("SelMainEnd");

  /* stop accounting and harvest info */
  AccountingDtor(nap);

  /* report to host. call destructors. exit */
  ZeroVMLogDtor();
  NaClExit(0);

  /* Unreachable, but having the return prevents a compiler error. */
  return -1;
}
