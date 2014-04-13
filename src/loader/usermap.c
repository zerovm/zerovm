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

#include <assert.h>
#include <sys/resource.h>
#include "src/loader/usermap.h"
#include "src/main/config.h"
#include "src/platform/sel_memory.h"

/* contains PROT_* bits of user pages */
static uint8_t user_map[USER_MAP_SIZE];

uint8_t *GetUserMap()
{
  return user_map;
}

void LockRestrictedMemory()
{
  int i;

  /* NULL and hole (everything protected with PROT_NONE is restricted) */
  for(i = 0; i < USER_MAP_SIZE; ++i)
    if(user_map[i] == PROT_NONE)
      user_map[i] = PROT_LOCK;
}

/* return index of "addr" in user map */
INLINE static int UserMapIndex(uintptr_t addr)
{
  return (addr - (uintptr_t)R15_CONST - GUARDSIZE) / NACL_MAP_PAGESIZE;
}

int CheckUserMap(uintptr_t addr, uint32_t size, int prot)
{
  int i;

  /* check arguments sanity */
  if(addr - MEM_START + size > FOURGIG)
    return -1;
  if(prot < PROT_NONE || prot > (PROT_READ|PROT_WRITE|PROT_EXEC))
    return -1;

  /* scan given region for availability */
  if(prot == 0)
  {
    for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
      if(user_map[UserMapIndex(addr) + i] & PROT_LOCK)
        return -1;
  }
  /* scan given region for prot */
  else
    for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
      if((user_map[UserMapIndex(addr) + i] & prot) == 0)
        return -1;

  return 0;
}

int UpdateUserMap(uintptr_t addr, uint32_t size, int prot)
{
  int i;

  /* check if all pages available */
  if(CheckUserMap(addr, size, PROT_NONE) != 0) return -1;

  /* set new protection for all pages */
  for(i = 0; i < ROUNDUP_64K(size) / NACL_MAP_PAGESIZE; ++i)
    user_map[UserMapIndex(addr) + i] = (uint8_t)prot;
  return 0;
}
