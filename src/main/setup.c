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
#include <stdio.h>
#include <assert.h>
#include <sys/resource.h>
#include "src/main/zlog.h"
#include "src/main/config.h"
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/syscalls/switch_to_app.h"
#include "src/syscalls/snapshot.h"
#include "src/syscalls/ztrace.h"
#include "src/syscalls/trap.h"
#include "src/channels/channel.h"
#include "src/platform/signal.h"
#include "src/platform/qualify.h"
#include "src/loader/usermap.h"
#include "src/loader/userspace.h"
#include "src/loader/uboot.inc" /* auto generated, do not modify */

static struct Manifest *s_manifest;

struct Manifest *GetManifest()
{
  return s_manifest;
}

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

int Validate(uint8_t *mbase, size_t size, uint32_t vbase)
{
  /* validator function from libvalidator.so */
  int NaClSegmentValidates(uint8_t *mbase, size_t size, uint32_t vbase);

  if(CommandPtr()->skip_validation > 1)
  {
    if(--CommandPtr()->skip_validation == 1)
      CommandPtr()->skip_validation = 0;
    return 0;
  }

  return CommandPtr()->skip_validation == 1 ?
      0 : 0 - !NaClSegmentValidates(mbase, size, vbase);
}

/* log user memory map */
static void LogMemMap()
{
  int i;
  uint8_t *map = GetUserMap();
  int prev = 0;
  int prev_prot = map[0];
  int cur_prot;

  ZLOGS(LOG_ERROR, "user memory map:");
  for(i = 1; i < FOURGIG / NACL_MAP_PAGESIZE; ++i)
  {
    cur_prot = map[i];
    if(prev_prot != cur_prot)
    {
      ZLOGS(LOG_ERROR, "%08X:%08X %x", NACL_MAP_PAGESIZE * prev,
          NACL_MAP_PAGESIZE * i - 1, prev_prot);
      prev = i;
      prev_prot = cur_prot;
    }
  }

  ZLOGS(LOG_ERROR, "%08X:%08X %x", FOURGIG - STACK_SIZE, FOURGIG - 1, map[i - 1]);
}

static void FinalDump()
{
  ZLOGS(LOG_INSANE, "exiting -- printing NaClApp details");
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
  i = Validate(buffer, size, NaClSysToUser((uintptr_t)buffer));
  ZLOGFAIL(i != 0, ENOEXEC, "boot validation failed");
  i = Zmprotect(buffer, ROUNDUP_64K(size), PROT_READ | PROT_EXEC);
  ZLOGFAIL(i != 0, EFAULT, "cannot protect boot");
  ThreadContextCtor(nacl_user, NaClSysToUser((uintptr_t)buffer), UserStackPtr());
}

void SessionCtor(char *mft)
{
  struct Manifest *manifest;

  /* initialize globals, ztrace and manifest */
  nacl_user = g_malloc(sizeof *nacl_user);
  nacl_sys = g_malloc(sizeof *nacl_sys);

  ZTraceCtor();
  manifest = ManifestCtor(mft);
  s_manifest = manifest;
  ZLOGS(LOG_DEBUG, "[manifest parsed]");
  ZTrace("[manifest parsed]");

  /* test platform and install signal handler after that */
  if(CommandPtr()->skip_qualification == 0)
    RunSelQualificationTests();
  SignalHandlerInit();
  ZLOGS(LOG_DEBUG, "[platform tested]");
  ZTrace("[platform tested]");

  /* create user space */
  MakeUserSpace();
  ZLOGS(LOG_DEBUG, "[user space created]");
  ZTrace("[user space created]");

  /* initialize all channels */
  ChannelsCtor(manifest);
  ZLOGS(LOG_DEBUG, "[channels constructed]");
  ZTrace("[channels constructed]");

  /* "defense in depth" */
  LastDefenseLine(manifest);
  ZLOGS(LOG_DEBUG, "[duck'n'covered]");
  ZTrace("[duck'n'covered]");

  /* set untrusted context switcher */
  InitSwitchToApp();
  ZLOGS(LOG_DEBUG, "[context switch initialized]");
  ZTrace("[context switch initialized]");

  /* construct trusted context */
  ThreadContextCtor(nacl_sys, 1, GetStackPtr());

  /* boot session from.. */
  if(g_strcmp0(manifest->boot, ".") != 0)
  /* ..scratch */
  {
    /* set user space areas */
    SetUserSpace(manifest);
    ZLOGS(LOG_DEBUG, "[user space set]");
    ZTrace("[user space set]");
    Boot(manifest);
    ZLOGS(LOG_DEBUG, "[booted]");
    ZTrace("[booted]");
  }
  /* ..from image */
  else
    LoadSession(manifest);

  fflush(NULL);
}

void SessionDtor(int code, char *state)
{
  ReportSetupPtr()->zvm_code = code;
  ReportSetupPtr()->zvm_state = g_strdup(state);

  /* broken session */
  if(ReportSetupPtr()->zvm_code != 0)
  {
    ZLOGS(LOG_ERROR, "SESSION %s FAILED WITH ERROR %d: %s",
        s_manifest == NULL ? "unknown" : s_manifest->node,
            ReportSetupPtr()->zvm_code, strerror(ReportSetupPtr()->zvm_code));
    FinalDump();
    ZTrace("[final dump]");
  }

  Report(s_manifest);
  ZTrace("[report]");
  ChannelsDtor(s_manifest);
  ZTrace("[channels destructed]");

  ZTrace("[untrusted context closed]");
  ManifestDtor(s_manifest); /* dispose manifest and channels */
  ZTrace("[manifest deallocated]");
  FreeUserSpace();
  ZTrace("[user space deallocated]");
  ZLogDtor();
  ZTrace("[zlog deallocated]");

  /* free local resources and exit */
  g_free(ReportSetupPtr()->zvm_state);
  g_free(ReportSetupPtr()->cmd);

  /* deallocate globals */
  g_free(nacl_sys);
  g_free(nacl_user);
  nacl_sys = NULL;
  nacl_user = NULL;

  ZTrace("[exit]");
  ZTraceDtor(1);
  _exit(ReportSetupPtr()->zvm_code);
}
