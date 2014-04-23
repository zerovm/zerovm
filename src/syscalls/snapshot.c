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
 * TODO(d'b): under construction
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "src/loader/sel_ldr.h"
#include "src/main/zlog.h"
#include "src/main/setup.h"
#include "src/syscalls/snapshot.h"
#include "src/loader/userspace.h"
#include "src/loader/usermap.h"
#include "src/channels/serializer.h"

#define R_FLAG (O_RDONLY)
#define W_MODE (S_IRUSR | S_IWUSR)
#define ID_MAP "map"
#define ID_DUMP "dump"
#define ID_CONTEXT "context"
#define ID_CHANNELS "channels"

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

/* save memory map to image */
static int SaveUserMap(void *map)
{
  int handle;

  handle = CreateImage(ID_MAP);
  if(handle < 0) return -1;

  if(WriteToImage(handle, map, USER_MAP_SIZE) != USER_MAP_SIZE)
    return -1;

  /* close image */
  return CloseImage(handle);
}

/* save user memory dump */
static int SaveDump(struct NaClApp *nap, uint8_t *map)
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

/* save user context to image */
static int SaveUserContext(struct ThreadContext *nacl_user)
{
  int handle;

  handle = CreateImage(ID_CONTEXT);
  if(handle < 0) return -1;

  if(WriteToImage(handle, nacl_user, sizeof *nacl_user) != sizeof *nacl_user)
    return -1;

  /* close image */
  return CloseImage(handle);
}

/* save text manifest to image */
static int SaveChannels(struct NaClApp *nap)
{
  int handle;
  struct ChannelsSerial *buffer;

  handle = CreateImage(ID_CHANNELS);
  if(handle < 0) return -1;

  /* write all available pages */
  buffer = ChannelsSerializer(nap->manifest, 0);
  if(WriteToImage(handle, buffer->channels, buffer->size) != buffer->size)
    return -1;

  /* close image */
  g_free(buffer);
  return CloseImage(handle);
}

/* load memory map from image to user space */
static int LoadUserMap(void *map)
{
  int handle;

  handle = OpenImage(ID_MAP);
  if(handle < 0) return -1;

  if(ReadFromImage(handle, map, USER_MAP_SIZE) != USER_MAP_SIZE)
    return -1;

  /* close image */
  return CloseImage(handle);
}

/* allocate user space (as usual, in safe way) and load dump */
static int LoadDump(struct NaClApp *nap, void *map)
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

/* load user context from image to user space */
static int LoadUserContext(struct ThreadContext *nacl_user)
{
  int handle;

  handle = OpenImage(ID_CONTEXT);
  if(handle < 0) return -1;

  if(ReadFromImage(handle, nacl_user, sizeof *nacl_user) != sizeof *nacl_user)
    return -1;

  /* close image */
  return CloseImage(handle);
}

static int LoadChannels(struct NaClApp *nap)
{
  int handle;
  uint32_t size;
  struct ChannelRec *channels;

  /* calculate and allocate memory for channels */
  size = GetFileSize(ID_CHANNELS);
  if(size < 0) return -1;
  channels = g_malloc(size);

  /* get channels data */
  handle = OpenImage(ID_CHANNELS);
  if(handle < 0) return -1;
  if(ReadFromImage(handle, channels, size) != size)
    return -1;

  /* set manifest from channels data */
  if(ChannelsDeserializer(nap->manifest, channels) != 0)
    return -1;

  /* close image */
  g_free(channels);
  return CloseImage(handle);
}

int SaveSession(struct NaClApp *nap)
{
  void *map = GetUserMap();

  if(SaveUserMap(map) < 0) return -1;
  if(SaveDump(nap, map) < 0) return -1;
  if(SaveUserContext(nacl_user) < 0) return -1;
  if(SaveChannels(nap) < 0) return -1;

  /* TODO(d'b): save report / session settings */

  return 0;
}

int LoadSession(struct NaClApp *nap)
{
  uint8_t map[USER_MAP_SIZE];

  if(LoadUserMap(map) < 0) return -1;
  if(LoadDump(nap, map) < 0) return -1;
  if(LoadUserContext(nacl_user) < 0) return -1;
  if(LoadChannels(nap) < 0) return -1;

  /* TODO(d'b): load report / session settings */

  return 0;
}
