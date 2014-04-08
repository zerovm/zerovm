/*
 * zerovm help screen, "last line" defense and user manifest serializer
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

#ifndef SETUP_H__
#define SETUP_H__ 1

EXTERN_C_BEGIN

#include "src/loader/sel_ldr.h"

#define HELP_SCREEN /* update command line switches here */\
    "%s%s\033[1m\033[37mZeroVM tag%d\033[0m lightweight VM manager, build 2014-04-08\n"\
    "Usage: <manifest> [-v#] [-T#] [-stFPQ]\n\n"\
    " -s skip validation\n"\
    " -t <0..2> report to stdout/log/fast (default 0)\n"\
    " -v <0..3> log verbosity (default 0)\n"\
    " -F quit right before starting user session\n"\
    " -P disable channels space preallocation\n"\
    " -Q disable platform qualification\n"\
    " -T enable trap calls tracing\n"

#define USER_MAP_SIZE (FOURGIG / NACL_MAP_PAGESIZE)
#define PROT_LOCK 0x8 /* protection of page cannot be changed */
#define ZEROVM_PRIORITY 19

/* validator function from libvalidator.so */
int NaClSegmentValidates(uint8_t* mbase, size_t size, uint32_t vbase);

/*
 * "defense in depth". the last frontier of defense.
 * zerovm limits itself as much as possible
 */
void LastDefenseLine();

/* preallocate memory area of given size. abort if fail */
/* TODO(d'b): move it to MemoryProtection() */
void PreallocateUserMemory(struct NaClApp *nap);

/* serialize system data to user space */
void SetSystemData(struct NaClApp *nap);

/* get user memory map */
uint8_t *GetUserMap();

/* lock user pages of: NULL, trampoline, hole, user manifest, stack */
/* TODO(d'b): ugly, magic numbers. heal the code */
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


EXTERN_C_END

#endif
