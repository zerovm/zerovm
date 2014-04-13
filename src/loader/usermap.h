/*
 * second part of user memory manager. controls protection for user space
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

#ifndef USERMAP_H_
#define USERMAP_H_

#include <stdint.h>
#include <sys/mman.h>

#define USER_MAP_SIZE (FOURGIG / NACL_MAP_PAGESIZE)
#define PROT_LOCK 0x8 /* protection of page cannot be changed */

/* get user memory map */
uint8_t *GetUserMap();

/* lock NULL and hole */
void LockRestrictedMemory();

/*
 * return 0 if memory region available or -1. aligned addr/size needed
 * NOTE: if prot == PROT_NONE function will check region for availability
 */
int CheckUserMap(uintptr_t addr, uint32_t size, int prot);

/*
 * update memory map with new protection. aligned addr/size needed.
 * returns 0 if successfully otherwise -1.
 * NOTE: can be used by user request and for internal zerovm needs
 */
int UpdateUserMap(uintptr_t addr, uint32_t size, int prot);

#endif /* USERMAP_H_ */
