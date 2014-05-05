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
#include "src/syscalls/switch_to_app.h"
#include "src/syscalls/snapshot.h"
#include "src/syscalls/ztrace.h"
#include "src/syscalls/trap.h"
#include "src/channels/channel.h"
#include "src/platform/signal.h"
#include "src/platform/qualify.h"
#include "src/loader/uboot.h"

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

/* set user stack and return its pointer (system address) */
static uintptr_t UserStackPtr()
{
  uintptr_t stack_ptr;

  /* calculate stack address */
  stack_ptr = MEM_START + ((uintptr_t)1U << ADDR_BITS);
  stack_ptr -= STACK_USER_DATA_SIZE;

  /* set argc, argv, envp */
  memset((void*)stack_ptr, 0, STACK_USER_DATA_SIZE);
  ((uint32_t*)stack_ptr)[4] = 1;
  ((uint32_t*)stack_ptr)[5] = 0xfffffff0;

  return stack_ptr;
}

/* load boot to the user memory */
static void Boot(struct Manifest *manifest)
{
  uint32_t size = uboot_bin_len;
  uint8_t *buffer;
  int i;

  assert(manifest != NULL);

  /* load user provided boot */
  if(manifest->boot != NULL)
  {
    int handle;

    size = GetFileSize(manifest->boot);
    ZLOGFAIL(size < 0, EIO, "cannot get boot size");
    buffer = (uint8_t*)UserHeapEnd() - ROUNDUP_64K(size);

    handle = open(manifest->boot, O_RDONLY);
    ZLOGFAIL(handle < 0, EIO, "cannot open %s", manifest->boot);
    ZLOGFAIL(read(handle, buffer, size) != size,
        EIO, "%s read error", manifest->boot);
  }
  /* load default boot */
  else
  {
    buffer = (uint8_t*)UserHeapEnd() - ROUNDUP_64K(size);
    memcpy(buffer, uboot_bin, size);
  }

  /* validate boot and set untrusted context */
  i = NaClSegmentValidates(buffer, size, NaClSysToUser((uintptr_t)buffer));
  ZLOGFAIL(i == 0, ENOEXEC, "boot validation failed");
  i = Zmprotect(buffer, ROUNDUP_64K(size), PROT_READ | PROT_EXEC);
  ZLOGFAIL(i != 0, EFAULT, "cannot protect boot");
  ThreadContextCtor(nacl_user, NaClSysToUser((uintptr_t)buffer), UserStackPtr());
}

void SessionCtor(struct NaClApp *nap, char *mft)
{
  /* initialize globals and manifest */
  NaClAppCtor(nap);
  g_nap = nap;
  nap->manifest = ManifestCtor(mft);

  /* test platform and install signal handler after that */
  if(CommandPtr()->skip_qualification == 0)
    RunSelQualificationTests();
  SignalHandlerInit();

  /* create user space */
  MakeUserSpace();

  /* initialize all channels */
  ChannelsCtor(nap->manifest);
  ZLOGS(LOG_DEBUG, "channels constructed");
  ZTrace("[channels mounting]");

  /* "defense in depth" */
  ZLOGS(LOG_DEBUG, "Last preparations");
  LastDefenseLine(nap->manifest);
  ZTrace("[last preparations]");

  /* set untrusted context switcher */
  InitSwitchToApp(nap);

  /* boot session from.. */
  if(g_strcmp0(nap->manifest->boot, ".") != 0)
  /* ..scratch */
  {
    /* set user space areas */
    SetUserSpace();
    Boot(nap->manifest);
  }
  /* ..from image */
  else
    LoadSession(nap);
  ZTrace("[session loaded]");

  /* construct trusted context */
  ThreadContextCtor(nacl_sys, 1, GetStackPtr());
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
