/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
#include <errno.h>
#include <assert.h>
#include "src/platform/nacl_exit.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/nacl_all_modules.h"

static int verb = LOG_INSANE;
static const char *zvm_state = "unknown error, see syslog";

static void VmentryPrinter(void *state, struct NaClVmmapEntry *vmep)
{
  UNREFERENCED_PARAMETER(state);
  ZLOG(verb, "page num 0x%06x", (uint32_t) vmep->page_num);
  ZLOG(verb, "num pages %d", (uint32_t) vmep->npages);
  ZLOG(verb, "prot bits %x", vmep->prot);
  fflush(stdout);
}

static void PrintVmmap(struct NaClApp *nap, int verbosity)
{
  ZLOG(verbosity, "In PrintVmmap");
  fflush(stdout);
  verb = verbosity;
  NaClVmmapVisit(&nap->mem_map, VmentryPrinter, NULL);
}

static void FinalDump(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "exiting -- printing NaClApp details\n");

  /* NULL can be used because syslog used for nacl log */
  NaClAppPrintDetails(nap, (struct Gio *) NULL, LOG_INSANE);
  ZLOGS(LOG_INSANE, "Dumping vmmap");
  PrintVmmap(nap, LOG_INSANE);
  ZLOGS(LOG_INSANE, "Done");

  if(nap->handle_signals) NaClSignalHandlerFini();
  NaClAllModulesFini();
}

/*
 * d'b: show dump (if needed). release resources, close channels.
 * note: use global nap because can be invoked from signal handler
 */
static void Finalizer(void)
{
  /* todo(d'b): get rid of hardcoded "ok" */
  if(!STREQ(zvm_state, OK_STATE)) FinalDump(gnap);

  SystemManifestDtor(gnap);
  ProxyReport(gnap);
}

/* set the text for "exit state" in report */
void SetExitState(const char *state)
{
  assert(state != NULL);
  zvm_state = state;
}

/* get the "exit state" message */
const char *GetExitState()
{
  assert(zvm_state != NULL);
  return zvm_state;
}

void NaClAbort(void)
{
  Finalizer();
  ZLOGIF(1, "zerovm aborted with no specific error code");
  _exit(EFAULT);
}

void NaClExit(int err_code)
{
  Finalizer();
  ZLOGIF(err_code != 0, "zerovm exited with error '%s'", strerror(err_code));
  _exit(err_code); /* supposed to be in sync with gnap->zvm_code */
}
