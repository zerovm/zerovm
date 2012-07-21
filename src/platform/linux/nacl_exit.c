/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#include <stdlib.h>
#include <unistd.h>

#include "src/platform/nacl_exit.h"
#include "src/manifest/manifest_setup.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_signal.h"
#include "src/service_runtime/nacl_all_modules.h"
#include "src/service_runtime/sel_mem.h"
#include "src/platform/nacl_log.h"

static void VmentryPrinter(void *state, struct NaClVmmapEntry *vmep)
{
  UNREFERENCED_PARAMETER(state);
  NaClLog(LOG_INFO, "page num 0x%06x\n", (uint32_t) vmep->page_num);
  NaClLog(LOG_INFO, "num pages %d\n", (uint32_t) vmep->npages);
  NaClLog(LOG_INFO, "prot bits %x\n", vmep->prot);
  fflush(stdout);
}

static void PrintVmmap(struct NaClApp *nap)
{
  NaClLog(LOG_INFO, "In PrintVmmap\n");
  fflush(stdout);
  NaClVmmapVisit(&nap->mem_map, VmentryPrinter, NULL);
}

static void FinalDump(struct NaClApp *nap)
{
  if(nap->verbosity)
  {
    NaClLog(LOG_INFO, "exiting -- printing NaClApp details\n");

    /* NULL can be used because syslog used for nacl log */
    NaClAppPrintDetails(nap, (struct Gio *) NULL);
    NaClLog(LOG_INFO, "Dumping vmmap.\n");
    PrintVmmap(nap);
    NaClLog(LOG_INFO, "Done.\n");
  }

  if(nap->handle_signals) NaClSignalHandlerFini();
  NaClAllModulesFini();
}

void NaClAbort(void)
{
  /*
   * d'b: finalize. show dump (if needed). release resources, close channels.
   * note: use global nap because can be invoked from signal handler
   */
  if(gnap->system_manifest->user_state != NULL)
    FinalDump(gnap);
  SystemManifestDtor(gnap);
  ProxyReport(gnap);

  abort();
}

void NaClExit(int err_code)
{
  /*
   * d'b: finalize. show dump (if needed). release resources, close channels.
   * note: use global nap because can be invoked from signal handler
   */
  if(gnap->system_manifest->user_state != NULL)
    FinalDump(gnap);
  SystemManifestDtor(gnap);
  ProxyReport(gnap);

  _exit(err_code);
}
