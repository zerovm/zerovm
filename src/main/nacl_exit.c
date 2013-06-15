/*
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */
/*
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

#include <errno.h>
#include <assert.h>
#include "src/main/nacl_exit.h"
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h"
#include "src/main/nacl_globals.h"
#include "src/platform/nacl_signal.h"
#include "src/main/accounting.h"
#include "src/loader/sel_addrspace.h"
#include "src/main/etag.h"
#include "src/channels/mount_channel.h"

static const char *zvm_state = UNKNOWN_STATE;
static int zvm_code = 0;

/* log user memory map */
static void LogMemMap(struct NaClApp *nap, int verbosity)
{
  int i;

  ZLOGS(verbosity, "user memory map (in pages):");

  for(i = 0; i < MemMapSize; ++i)
  {
    ZLOGS(verbosity, "%s: address = 0x%08x, size = 0x%08x, protection = x",
        nap->mem_map[i].name, (uint32_t) nap->mem_map[i].start,
        (uint32_t) nap->mem_map[i].size, nap->mem_map[i].prot);
  }
}

static void FinalDump(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "exiting -- printing NaClApp details");

  /* NULL can be used because syslog used for nacl log */
  NaClAppPrintDetails(nap, (struct Gio *) NULL, LOG_INSANE);
  LogMemMap(nap, LOG_INSANE);

  NaClSignalHandlerFini();
}

/*
 * d'b: show dump (if needed). release resources, close channels.
 * note: use global nap because can be invoked from signal handler
 */
static void Finalizer(void)
{
  if(!STREQ(zvm_state, OK_STATE)) FinalDump(gnap);

  SystemManifestDtor(gnap); /* finalize channels */
  AccountingDtor(gnap); /* get accounting */
  ProxyReport(gnap); /* show report */
  ChannelsDtor(gnap); /* free channels */
  NaClAppDtor(gnap); /* free user space and globals */
  NaClFreeDispatchThunk(gnap); /* free thunk */
  ZLogDtor(); /* close syslog */
  ManifestDtor(); /* free manifest */
}

void SetExitState(const char *state)
{
  assert(state != NULL);
  zvm_state = state;
}

const char *GetExitState()
{
  assert(zvm_state != NULL);
  return zvm_state;
}

void SetExitCode(int code)
{
  /* only the 1st error matters */
  if(zvm_code == 0) zvm_code = code;
}

int GetExitCode()
{
  return zvm_code;
}

/* manage special signals. note: may change zvm_state */
/* todo(d'b): remove this patch after reworking zvm signals */
static void SpecSignals()
{
  if(strstr(zvm_state, "Signal 14") != NULL)
    SetExitState("session timeout");
  else if(strstr(zvm_state, "Signal 25") != NULL)
    SetExitState("disk quota exceeded");
}

void NaClExit(int err_code)
{
  SetExitCode(err_code);

  /*
   * patch to show special messages instead of signals 24, 25
   * and do not calculate etag if session failed
   */
  if(err_code != 0)
  {
    SpecSignals();
    ZLOGS(LOG_ERROR, "SESSION OR HYPERVISOR FAILED WITH ERROR %d: %s",
        err_code, strerror(zvm_code));
  }

  Finalizer();
  _exit(zvm_code);
}
