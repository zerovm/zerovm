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

#define R_FLAG (O_RDONLY)
#define W_MODE (S_IRUSR | S_IWUSR)
#define ID_MAP "map"
#define ID_DUMP "dump"
#define ID_CONTEXT "context"
#define ID_CHANNELS "channels"

#if 0 // ### disabled as useless
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
#endif

#if 0
/* write "buffer" to disk under "id" name */
static int WriteData(char *name, void *buffer, uint32_t size)
{
  int handle;

  /* open id part of image */
  handle = creat(name, W_MODE);
  if(handle < 0) return -1;

  /* write data */
  if(write(handle, buffer, size) != size)
  {
    close(handle);
    unlink(name);
    return -1;
  }

  /* close file */
  if(close(handle) < 0) return -1;
  return 0;
}
#endif

/*
 * wrappers {{
 */
/* create file for "name" and  return handle or -1 */
static int CreateImage(char *name)
{
  return creat(name, W_MODE);
}

/* write "buffer" to "handle" */
static int AppendToImage(int handle, void *buffer, uint32_t size)
{
  return write(handle, buffer, size);
}

/* open image file for "name" and return handle or -1 */
static int OpenImage(char *name)
{
  return open(name, R_FLAG);
}

/* read to "buffer" from "handle" */
static int ReadFromImage(int handle, void *buffer, uint32_t size)
{
  return read(handle, buffer, size);
}

/* close image handle */
static int CloseImage(int handle)
{
  return close(handle);
}
/* wrappers }} */

/* save memory map to image */
static int SaveUserMap(void *map)
{
  int handle;

  handle = CreateImage(ID_MAP);
  if(handle < 0) return -1;

  if(AppendToImage(handle, map, USER_MAP_SIZE) != USER_MAP_SIZE)
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
      if(AppendToImage(handle, (void*)MEM_START + i * NACL_MAP_PAGESIZE,
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

  if(AppendToImage(handle, nacl_user, sizeof *nacl_user) != sizeof *nacl_user)
    return -1;

  /* close image */
  return CloseImage(handle);
}

/* save text manifest to image */
// ### serializer needed
static int SaveChannels(struct NaClApp *nap)
{
  int i;
  int handle;

  handle = CreateImage(ID_CHANNELS);
  if(handle < 0) return -1;

  /* write all available pages */
  for(i = 0; i < nap->manifest->channels->len; ++i)
  {
    struct ChannelDesc channel;
//    if(AppendToImage(handle, (void*)CH_CH(nap->manifest, i),
//        NACL_MAP_PAGESIZE) != NACL_MAP_PAGESIZE)
    return -1;
  }

  /* close image */
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
/* TODO(d'b): this logic should replace the one inherited from NaCl */
// ###
static int LoadDump(struct NaClApp *nap, void *map)
{
  /* open dump file */
  /* allocate user space */
  /* load data mprotecting it */
  /* close dump file */

  return -1;
}

/* load user context from image to user space */
// ###
static int LoadUserContext(struct ThreadContext *nacl_user)
{
  return -1;
}

// ###
static int LoadChannels(struct NaClApp *nap)
{
  return -1;
}

#if 0
/* read old manifest from image, and check it up against the new one */
static void CheckManifest(struct NaClApp *nap)
{
}
#endif

int SaveSession(struct NaClApp *nap)
{
  void *map = GetUserMap();

  if(SaveUserMap(map) < 0) return -1;
  if(SaveDump(nap, map) < 0) return -1;
  if(SaveUserContext(nacl_user) < 0) return -1;
  if(SaveChannels(nap) < 0) return -1;

  return 0;
}

int LoadSession(struct NaClApp *nap)
{
  void *map = GetUserMap();

  if(LoadUserMap(map) < 0) return -1;
  if(LoadDump(nap, map) < 0) return -1;
  if(LoadUserContext(nacl_user) < 0) return -1;
  if(LoadChannels(nap) < 0) return -1;

  return 0;
}
