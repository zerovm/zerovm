/*
 * construct system manifest and host manifest
 * system/channels resources counters are also here
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
#include <sys/resource.h>
#include "src/loader/sel_ldr.h"
#include "src/loader/usermap.h"
#include "src/loader/userspace.h"
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/syscalls/snapshot.h"
#include "src/syscalls/ztrace.h"
#include "src/channels/channel.h"
#include "src/platform/signal.h"
#include "src/platform/qualify.h"

static struct NaClApp *g_nap = NULL;

/* set timeout. by design timeout must be specified in manifest */
static void SetTimeout(struct Manifest *manifest)
{
  assert(manifest != NULL);

  ZLOGFAIL(manifest->timeout < 1, EFAULT, "invalid timeout %d", manifest->timeout);
  alarm(manifest->timeout);
}

/* lower zerovm priority */
static void LowerOwnPriority()
{
  int i = setpriority(PRIO_PROCESS, 0, ZEROVM_PRIORITY);
  ZLOGFAIL(i != 0, errno, "cannot set zerovm priority");
}

/*
 * give up privileges
 * TODO(d'b): instead of fail set group/user to nogroup/nobody
 */
static void GiveUpPrivileges()
{
  ZLOGFAIL(getuid() == 0, EPERM, "zerovm is not permitted to run as root");
}

void LastDefenseLine(struct Manifest *manifest)
{
  SetTimeout(manifest);
  LowerOwnPriority();
  GiveUpPrivileges();
}

static void ValidateProgram(struct NaClApp *nap)
{
  int status = 0; /* 0 = failed, 1 = successful */
  int64_t size;
  uint8_t *addr;

  assert(nap->static_text_end > 0);

  /* static and dynamic text address / length */
  size = nap->static_text_end - NaClSysToUser(MEM_START + NACL_TRAMPOLINE_END);
  addr = (uint8_t*)NaClUserToSys(NACL_TRAMPOLINE_END);

  /* validate static and dynamic text */
  if(size > 0)
    status = NaClSegmentValidates(addr, size, nap->initial_entry_pt);

  /* set results */
  ReportSetupPtr()->validation_state = 1;
  ZLOGFAIL(status == 0, ENOEXEC, "validation failed");
  ReportSetupPtr()->validation_state = 0;
}

/* log user memory map */
/* TODO(d'b): rewrite or remove. this function spams syslog */
static void LogMemMap()
{
  int i;
  uint8_t *map = GetUserMap();
  int prev = 0;
  int prev_prot = map[0];
  int cur_prot;

  ZLOGS(LOG_DEBUG, "user memory map:");
  for(i = 1; i < FOURGIG / NACL_MAP_PAGESIZE; ++i)
  {
    cur_prot = map[i];
    if(prev_prot != cur_prot)
    {
      ZLOGS(LOG_DEBUG, "%08X:%08X %x", NACL_MAP_PAGESIZE * prev,
          NACL_MAP_PAGESIZE * i - 1, prev_prot);
      prev = i;
      prev_prot = cur_prot;
    }
  }

  ZLOGS(LOG_DEBUG, "%08X:%08X %x", FOURGIG - STACK_SIZE, FOURGIG - 1, map[i - 1]);
}

static void FinalDump(struct NaClApp *nap)
{
  ZLOGS(LOG_INSANE, "exiting -- printing NaClApp details");
  PrintAppDetails(nap, LOG_INSANE);
  LogMemMap();

  SignalHandlerFini();
}

void SessionCtor(struct NaClApp *nap, char *mft)
{
  struct GioMemoryFileSnapshot boot;

  /* initialize globals */
  NaClAppCtor(nap);

  /* initialize manifest */
  g_nap = nap;
  nap->manifest = ManifestCtor(mft);

  /* test platform and install signal handler after that */
  if(CommandPtr()->skip_qualification == 0)
    RunSelQualificationTests();
  SignalHandlerInit();

  if(g_strcmp0(nap->manifest->program, ".") != 0)
  /* create session from scratch */
  {
    /* read elf into memory */
    ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&boot, nap->manifest->program),
        ENOENT, "open '%s': %s", nap->manifest->program, strerror(errno));
    ZTrace("[memory snapshot]");

    /* initialize all channels */
    /* TODO(d'b): should be done *after* validation */
    ChannelsCtor(nap->manifest);
    ZLOGS(LOG_DEBUG, "channels constructed");
    ZTrace("[channels mounting]");

    /* validate program structure (check elf header and segments) */
    ZLOGS(LOG_DEBUG, "Loading %s", nap->manifest->program);
    AppLoadFile((struct Gio *) &boot, nap);
    ZTrace("[user module loading]");

    /* validate given program (ensure that text segment is safe) */
    ZLOGS(LOG_DEBUG, "Validating %s", nap->manifest->program);
    if(CommandPtr()->skip_validation == 0)
      ValidateProgram(nap);
    ZTrace("[user module validation]");

    /* free snapshot */
    if(-1 == (*((struct Gio*)&boot)->vtbl->Close)((struct Gio *)&boot))
      ZLOG(LOG_ERROR, "Error while closing '%s'", nap->manifest->program);
    (*((struct Gio*)&boot)->vtbl->Dtor)((struct Gio*)&boot);
    ZTrace("[snapshot deallocation]");

    /* lock restricted regions in user memory */
    LockRestrictedMemory();
    ZLOGS(LOG_DEBUG, "lock restricted user regions");
    ZTrace("[restricted user regions locking]");
  }
  /* load session from image */
  else
  {
    /* initialize all channels */
    ChannelsCtor(nap->manifest);
    ZLOGS(LOG_DEBUG, "channels constructed");
    ZTrace("[channels mounting]");

    /* load session */
    LoadSession(nap);
    ZLOGS(LOG_DEBUG, "session loading");
    ZTrace("[session loading]");
  }

  /* "defense in depth" call */
  ZLOGS(LOG_DEBUG, "Last preparations");
  LastDefenseLine(nap->manifest);
  ZTrace("[last preparations]");
}

void SessionDtor(int code, char *state)
{
  ReportSetupPtr()->zvm_code = code;
  ReportSetupPtr()->zvm_state = g_strdup(state);

  /* broken session */
  if(ReportSetupPtr()->zvm_code != 0)
  {
    ZLOGS(LOG_ERROR, "SESSION %s FAILED WITH ERROR %d: %s",
        g_nap->manifest == NULL ? "unknown" : g_nap->manifest->node,
            ReportSetupPtr()->zvm_code, strerror(ReportSetupPtr()->zvm_code));
    FinalDump(g_nap);
    ZTrace("[final dump]");
  }

  Report(g_nap->manifest);
  ZTrace("[report]");
  ChannelsDtor(g_nap->manifest);
  ZTrace("[channels destructed]");
  NaClAppDtor(g_nap);
  ZTrace("[untrusted context closed]");
  ManifestDtor(g_nap->manifest); /* dispose manifest and channels */
  ZTrace("[manifest deallocated]");
  FreeUserSpace();
  ZTrace("[user space deallocated]");
  ZLogDtor();
  ZTrace("[zlog deallocated]");

  /* free local resources and exit */
  g_free(ReportSetupPtr()->zvm_state);
  g_free(ReportSetupPtr()->cmd);

  ZTrace("[exit]");
  ZTraceDtor(1);
  ZTraceNameDtor();
  _exit(ReportSetupPtr()->zvm_code);
}
