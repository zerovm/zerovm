/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * ZeroVM main
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
#include <errno.h>
#include <glib.h>
#include "src/platform/gio.h"
#include "src/main/nacl_globals.h"
#include "src/platform/nacl_signal.h"
#include "src/main/etag.h"
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h"
#include "src/platform/sel_qualify.h"
#include "src/main/accounting.h"
#include "src/platform/nacl_macros.h"
#include "src/channels/preload.h" /* for PreloadAllocationDisable() */

#define BADCMDLINE(msg) \
  do { \
    printf("%s%s%s", msg == NULL ? "" : msg, \
      msg == NULL ? "" : "\n", HELP_SCREEN); exit(EINVAL); \
  } while(0)

static int skip_qualification = 0;
static int quit_after_load = 0;
static int skip_validator = 0;

/* log zerovm command line */
static void ZVMCommandLine(int argc, char **argv)
{
  char cmd[BIG_ENOUGH_STRING];
  int offset = 0;
  int i;

  offset += sprintf(cmd, "ZeroVM command line:");
  for(i = 0; i < argc; ++i)
    offset += g_snprintf(cmd + offset, BIG_ENOUGH_STRING - offset, " %s", argv[i]);

  ZLOGS(LOG_DEBUG, "%s", cmd);
}

/* parse given command line, parse manifest and initialize NaClApp object */
static void ParseCommandLine(struct NaClApp *nap, int argc, char **argv)
{
  int opt;
  char *manifest_name = NULL;

  /* construct zlog with default verbosity */
  ZLogCtor(LOG_ERROR);

  while((opt = getopt(argc, argv, "-PFQsSv:M:l:e:")) != -1)
  {
    switch(opt)
    {
      case 1:
      case 'M':
        if(manifest_name != NULL)
          BADCMDLINE("2nd manifest encountered");
        manifest_name = optarg;
        break;
      case 's':
        skip_validator = 1;
        ZLOGS(LOG_ERROR, "VALIDATION DISABLED by -s");
        break;
      case 'F':
        quit_after_load = 1;
        break;
      case 'e':
        if(TagEngineCtor(ATOI(optarg)) != 0)
          BADCMDLINE("invalid etag level");
        nap->user_tag = TagCtor();
        break;
      case 'S':
        /*
         * todo(d'b): this one can fail w/o showing help. fix it! or,
         * even better, remove it. zerovm always need to show report,
         * and therefore always should catch signals
         */
        SetSignalHandling(0);
        ZLOGS(LOG_ERROR, "SIGNAL HANDLING DISABLED by -S");
        break;
      case 'l':
        /* calculate hard limit in Gb and don't allow it less then "big enough" */
        if(SetStorageLimit(ATOI(optarg)) != 0)
          BADCMDLINE("invalid storage limit");
        break;
      case 'v':
        ZLogDtor();
        ZLogCtor(ATOI(optarg));
        break;
      case 'Q':
        skip_qualification = 1;
        ZLOGS(LOG_ERROR, "PLATFORM QUALIFICATION DISABLED by -Q");
        break;
      case 'P':
        ZLOGS(LOG_ERROR, "DISK SPACE PREALLOCATION DISABLED by -P");
        PreloadAllocationDisable();
        break;
      default:
        BADCMDLINE(NULL);
        break;
    }
  }

  /* show zerovm command line */
  ZVMCommandLine(argc, argv);

  /* last chance to find command line errors */
  if(manifest_name == NULL) BADCMDLINE(NULL);

  /* parse manifest file specified in command line */
  ZLOGFAIL(ManifestCtor(manifest_name), EFAULT, "Invalid manifest '%s'", manifest_name);

  /* set available nap and manifest fields */
  assert(nap->system_manifest != NULL);
  nap->system_manifest->nexe = GetValueByKey(MFT_NEXE);
  ZLOGFAIL(nap->system_manifest->nexe == NULL, EFAULT, "nexe not specified");
  ZLOGFAIL(GetFileSize(nap->system_manifest->nexe) < 0, ENOENT, "nexe open error");
}

#define STATIC_TEXT_START ((uintptr_t)0x20000)
int NaClSegmentValidates(uint8_t* mbase, size_t size, uint32_t vbase);
static void ValidateNexe(struct NaClApp *nap)
{
  int status = 0; /* 0 = failed, 1 = successful */
  int64_t static_size;
  int64_t dynamic_size;
  uint8_t* static_addr;
  uint8_t* dynamic_addr;

  assert((nap->static_text_end | nap->dynamic_text_start
      | nap->dynamic_text_end | nap->mem_start) > 0);

  /* skip validation if specified */
  if(skip_validator)
  {
    nap->validation_state = 2;
    return;
  }

  /* static and dynamic text address / length */
  static_size = nap->static_text_end -
      NaClSysToUser(nap, nap->mem_start + STATIC_TEXT_START);
  dynamic_size = nap->dynamic_text_end - nap->dynamic_text_start;
  static_addr = (uint8_t*)NaClUserToSys(nap, STATIC_TEXT_START);
  dynamic_addr = (uint8_t*)NaClUserToSys(nap, nap->dynamic_text_start);

  /* validate static and dynamic text */
  if(static_size > 0)
    status = NaClSegmentValidates(static_addr, static_size, nap->initial_entry_pt);
  if(dynamic_size > 0)
    status &= NaClSegmentValidates(dynamic_addr, dynamic_size, nap->initial_entry_pt);

  /* set results */
  nap->validation_state = 1;
  ZLOGFAIL(status == 0, ENOEXEC, "validation failed");
  nap->validation_state = 0;
}

int main(int argc, char **argv)
{
  struct NaClApp state, *nap = &state;
  struct SystemManifest sys_mft;
  struct GioMemoryFileSnapshot main_file;
  GTimer *timer;

  /* zerovm initialization */
  memset(nap, 0, sizeof *nap);
  nap->system_manifest = &sys_mft;
  memset(nap->system_manifest, 0, sizeof *nap->system_manifest);
  gnap = nap;

  ParseCommandLine(nap, argc, argv);

  /* We use the signal handler to verify a signal took place. */
  if(skip_qualification == 0) NaClRunSelQualificationTests();

  NaClSignalHandlerInit();

  /* initialize mem_map and set nap fields to default values */
  ZLOGFAIL(NaClAppCtor(nap) == 0, EFAULT, "Error while constructing app state");

  /* read nexe into memory */
  timer = g_timer_new();
  ZLOGFAIL(0 == GioMemoryFileSnapshotCtor(&main_file, nap->system_manifest->nexe),
      ENOENT, "Cannot open '%s'. %s", nap->system_manifest->nexe, strerror(errno));

#define TIMER_REPORT(msg) \
  do {\
    ZLOGS(LOG_DEBUG, "...TIMER: %s took %.3f milliseconds", msg,\
        g_timer_elapsed(timer, NULL) * NACL_MICROS_PER_MILLI);\
    g_timer_start(timer);\
  } while(0)

  TIMER_REPORT("constructing of memory snapshot");

  /* validate nexe structure (check elf header and segments) */
  ZLOGS(LOG_DEBUG, "Loading %s", nap->system_manifest->nexe);
  NaClAppLoadFile((struct Gio *) &main_file, nap);
  TIMER_REPORT("loading user module");

  /* validate given nexe (ensure that text segment is safe) */
  ZLOGS(LOG_DEBUG, "Validating %s", nap->system_manifest->nexe);
  ValidateNexe(nap);
  TIMER_REPORT("validating user module");

  /* free snapshot */
  if(-1 == (*((struct Gio *)&main_file)->vtbl->Close)((struct Gio *)&main_file))
    ZLOG(LOG_ERROR, "Error while closing '%s'", nap->system_manifest->nexe);
  (*((struct Gio *) &main_file)->vtbl->Dtor)((struct Gio *) &main_file);

  /* setup zerovm from manifest */
  ZLOGS(LOG_DEBUG, "Setting hypervisor");
  SystemManifestCtor(nap);
  TIMER_REPORT("setting hypervisor from manifest");

  /* "defence in depth" call */
  ZLOGS(LOG_DEBUG, "Last preparations");
  LastDefenseLine(nap);

  /* Make sure all the file buffers are flushed before entering the nexe */
  fflush((FILE*) NULL);

  /* start accounting */
  AccountingCtor(nap);

  /* quit if fuzz testing specified */
  if(quit_after_load)
  {
    SetExitState(OK_STATE);
    NaClExit(0);
  }
  TIMER_REPORT("last preparations");

  /* switch to the user code */
  ZLOGFAIL(!NaClCreateMainThread(nap), EFAULT, "switching to nexe failed");
  return EFAULT; /* unreachable */
}
