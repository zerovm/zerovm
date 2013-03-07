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
#include "src/main/manifest_setup.h"
#include "src/main/nacl_globals.h"
#include "src/platform/nacl_signal.h"
#include "src/main/accounting.h"
#include "src/main/etag.h"

static const char *zvm_state = UNKNOWN_STATE;
static int zvm_code = 0;

/* log user memory map */
static void LogMemMap(struct NaClApp *nap, int verbosity)
{
  int i;

  ZLOGS(verbosity, "user memory map (in pages):");

  for(i = 0; i < MemMapSize; ++i)
  {
    ZLOGS(verbosity, "%s: address = 0x%06x, size = %d, protection = x",
        nap->mem_map[i].name,
        (uint32_t) nap->mem_map[i].page_num,
        (uint32_t) nap->mem_map[i].npages,
        nap->mem_map[i].prot);
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

  AccountingDtor(gnap);
  SystemManifestDtor(gnap);
  ProxyReport(gnap);
  TagEngineDtor();
  ZLogDtor();
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

/* set zerovm exit code */
void SetExitCode(int code)
{
  /* only the 1st error matters */
  if(zvm_code == 0) zvm_code = code;
}

/* manage special signals. note: may change zvm_state */
/* todo(d'b): remove this patch after reworking zvm signals */
static void SpecSignals()
{
  if(strstr(zvm_state, "Signal 24") != NULL)
    SetExitState("session timeout");
  else if(strstr(zvm_state, "Signal 25") != NULL)
    SetExitState("disk quota exceeded");
}

void NaClExit(int err_code)
{
  /* patch */
  if(err_code !=0) SpecSignals();

  Finalizer();
  SetExitCode(err_code);

  ZLOGIF(zvm_code != 0, "zerovm exited with error '%s'", strerror(zvm_code));
  _exit(zvm_code);
}
