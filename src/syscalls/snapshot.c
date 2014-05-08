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

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#include "src/main/zlog.h"
#include "src/main/config.h"
#include "src/main/setup.h"
#include "src/main/report.h"
#include "src/main/manifest.h"
#include "src/syscalls/snapshot.h"
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/loader/context.h"
#include "src/channels/channel.h"
#include "src/channels/serializer.h"

#define R_FLAG (O_RDONLY)
#define W_MODE (S_IRUSR | S_IWUSR)
#define ID_MAP "map"
#define ID_DUMP "dump"
#define ID_CONTEXT "context"
#define ID_CHANNELS "channels"
#define ID_REPORT "report"
#define ID_SESSION "session"

/* create file for "name" and  return handle or -1 */
INLINE static int CreateImage(char *name)
{
  return creat(name, W_MODE);
}

/* write "buffer" to "handle" */
INLINE static int WriteToImage(int handle, void *buffer, uint32_t size)
{
  return write(handle, buffer, size);
}

/* open image file for "name" and return handle or -1 */
INLINE static int OpenImage(char *name)
{
  return open(name, R_FLAG);
}

/* read to "buffer" from "handle" */
INLINE static int ReadFromImage(int handle, void *buffer, uint32_t size)
{
  return read(handle, buffer, size);
}

/* close image handle */
INLINE static int CloseImage(int handle)
{
  return close(handle);
}

/* save fixed size data to image */
static int SaveFixedSize(char *name, void *buffer, int size)
{
  int handle = CreateImage(name);

  if(handle < 0) return -1;
  if(WriteToImage(handle, buffer, size) != size)
    return -1;
  return CloseImage(handle);
}

/* save user memory dump */
static int SaveDump(uint8_t *map)
{
  int handle;
  int i;

  /* open image to save dump */
  handle = CreateImage(ID_DUMP);
  if(handle < 0) return -1;

  /* write all available pages */
  for(i = 0; i < USER_MAP_SIZE; ++i)
    if(map[i] & (PROT_READ | PROT_WRITE | PROT_EXEC))
      if(WriteToImage(handle, (void*)MEM_START + i * NACL_MAP_PAGESIZE,
          NACL_MAP_PAGESIZE) != NACL_MAP_PAGESIZE)
        return -1;

  /* close image */
  return CloseImage(handle);
}

/* save fixed size data from image */
static int LoadFixedSize(char *name, void *buffer, int size)
{
  int handle = OpenImage(name);

  if(handle < 0) return -1;
  if(ReadFromImage(handle, buffer, size) != size)
    return -1;
  return CloseImage(handle);
}

/* allocate user space (as usual, in safe way) and load dump */
static int LoadDump(void *map)
{
  int handle;
  uintptr_t addr;
  uint32_t size;
  uint32_t idx = 0;
  uint32_t end;

  /* create user space */
  MakeUserSpace();

  /* open dump */
  handle = OpenImage(ID_DUMP);
  if(handle < 0) return -1;

  /* loop through all pages */
  do {
    /* detect pages sequence with same protection */
    for(end = idx + 1; end < USER_MAP_SIZE; ++end)
      if(((uint8_t*)map)[end - 1] != ((uint8_t*)map)[end]) break;

    size = NACL_MAP_PAGESIZE * (end - idx);
    addr = MEM_START + NACL_MAP_PAGESIZE * idx;

    /* load non-empty pages */
    if(((uint8_t*)map)[idx] != PROT_LOCK)
    {
      /* make pages writable */
      if(Zmprotect(TOPTR(addr), size, PROT_WRITE) != 0)
        return -1;

      /* read pages to user space */
      if(ReadFromImage(handle, TOPTR(addr), size) != size)
        return -1;
    }

    /* protect pages */
    if(Zmprotect(TOPTR(addr), size, ((uint8_t*) map)[idx]) != 0)
      return -1;
    idx = end;
  } while(end < USER_MAP_SIZE);

  return CloseImage(handle);
}

int SaveSession(struct Manifest *manifest)
{
  void *map = GetUserMap();
  struct ChannelsSerial *buffer = ChannelsSerializer(manifest, 0);

  if(SaveFixedSize(ID_MAP, map, USER_MAP_SIZE) < 0)
    return -1;
  if(SaveDump(map) < 0)
    return -1;
  if(SaveFixedSize(ID_CONTEXT, nacl_user, sizeof *nacl_user) < 0)
    return -1;

  /*
   * TODO(d'b): decide leave it or remove. seems that CommandPtr() can be
   * safely removed. ReportSetupPtr() contains information about session
   * state and perhaps it should be checked before snapshot, if only good
   * session will be allowed to store, ReportSetupPtr() can be removed
   * as well
   * NOTE: disabled until "allocation" bug will not be resolved
   */
#if 0
  if(SaveFixedSize(ID_REPORT, ReportSetupPtr(), sizeof *ReportSetupPtr()) < 0)
    return -1;
  if(SaveFixedSize(ID_SESSION, CommandPtr(), sizeof *CommandPtr()) < 0)
    return -1;
#endif

  if(SaveFixedSize(ID_CHANNELS, buffer->channels, buffer->size) < 0)
    return -1;

  g_free(buffer);
  return 0;
}

int LoadSession(struct Manifest *manifest)
{
  uint8_t map[USER_MAP_SIZE];
  uint32_t size = GetFileSize(ID_CHANNELS);
  struct ChannelRec *channels;

  if(LoadFixedSize(ID_MAP, map, USER_MAP_SIZE) < 0)
    return -1;
  if(LoadDump(map) < 0)
    return -1;
  if(LoadFixedSize(ID_CONTEXT, nacl_user, sizeof *nacl_user) < 0)
    return -1;

#if 0 /* see comment above */
  if(LoadFixedSize(ID_REPORT, ReportSetupPtr(), sizeof *ReportSetupPtr()) < 0)
    return -1;
  if(LoadFixedSize(ID_SESSION, CommandPtr(), sizeof *CommandPtr()) < 0)
    return -1;
#endif

  /* calculate and allocate memory for channels */
  if(size < 0) return -1;
  channels = g_malloc(size);

  /* set manifest from channels data */
  if(LoadFixedSize(ID_CHANNELS, channels, size) < 0)
    return -1;
  if(ChannelsDeserializer(manifest, channels) != 0)
    return -1;

  g_free(channels);
  return 0;
}
