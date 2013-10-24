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

/*
 * TODO(d'b): design (under construction)
 * image consist of 4 main parts:
 * 1. user memory map (for mprotect()'ion)
 * 2. user memory dump
 * 3. user context (registers) + user_ret (user entry point)
 * 4. manifest (text file, variable size, ends with eof)
 *
 * to save image
 * - check session status (should be "ok")
 * - check "Save" value
 * - catch TrapSave (drop, hold, shot, snap)
 * - store all mentioned above data into the file
 *
 * to restore image (zerovm initialization as usual)
 * - allocate user space
 * - read (to user space) memory map (it is 64kb)
 * - read (to user space) memory dump until hole (infer from map)
 * - read (to user space) the rest (user manifest and stack)
 * - read user context and initialize nacl_sys
 * - reconstruct nap from data above (nap->sysret to TrapSave retcode)
 * - return to user code (user_ret from image contain entry point)
 *
 * CHANGES IN ZEROVM DESIGN
 * - main() loading of program with all checks allocations should be wrapped and carried out
 * - CreateSession() should not set user stack and affected things
 * - add manifest text to nap or manifest
 */
#include <stdio.h>
#include <assert.h>
#include "src/loader/sel_ldr.h"
#include "src/main/zlog.h"
#include "src/syscalls/snapshot.h"

static int image = -1;

/*
 * return 0: given file contains session, -1: random file
 * note: initializes image handler
 */
#define MAGIC 0x3030474d494d565aULL
static int IsImage(const char *name)
{
  uint64_t magic = 0;
  int code;

  /* open image */
  if(image < 0)
    image = open(name, O_RDONLY);
  if(image < 0) return -1;

  /* read "magic" */
  code = read(image, &magic, sizeof magic);
  return code == sizeof magic ? magic == MAGIC ? 0 : -1 : -1;
}

/* get memory map from system */
static void *GetSystemMemoryMap()
{
  return NULL;
}

/* save memory map to image */
static int SaveMemoryMap(void *map)
{
  return -1;
}

/* save 1st part of user memory dump to image */
static int SaveMemory1(struct NaClApp *nap, void *map)
{
  return -1;
}

/* save 2nd part of user memory dump to image */
static int SaveMemory2(struct NaClApp *nap, void *map)
{
  return -1;
}

/* save user context to image */
static int SaveUserContext(struct NaClApp *nap)
{
  return -1;
}

/* save text manifest to image */
static int SaveManifest(struct NaClApp *nap)
{
  return -1;
}

/* .. */

/* load memory map from image to user space */
static void *LoadMemoryMap(struct NaClApp *nap)
{
  return NULL;
}

/* load 1st part of memory dump from image to user space */
static void LoadMemory1(struct NaClApp *nap, void *map)
{
}

/* load 2nd part of memory dump from image to user space */
static void LoadMemory2(struct NaClApp *nap, void *map)
{
}

/* load user context from image to user space */
static void LoadUserContext(struct NaClApp *nap)
{
}

/* read old manifest from image, and check it up against the new one */
static void CheckManifest(struct NaClApp *nap)
{
}

int SaveSession(struct NaClApp *nap)
{
  int code;
  void *map;

  map = GetSystemMemoryMap();
  if(map == NULL) return -1;

  code = SaveMemoryMap(map);
  if(code < 0) return -1;

  code = SaveMemory1(nap, map);
  if(code < 0) return -1;

  code = SaveMemory2(nap, map);
  if(code < 0) return -1;

  code = SaveUserContext(nap);
  if(code < 0) return -1;

  code = SaveManifest(nap);
  if(code < 0) return -1;

  return 0;
}

int LoadSession(struct NaClApp *nap, const char *name)
{
  void *map;
  if(IsImage(name) < 0) return -1;

  map = LoadMemoryMap(nap);
  LoadMemory1(nap, map);
  LoadMemory2(nap, map);
  LoadUserContext(nap);
  CheckManifest(nap);

  return 0;
}
