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
    "\033[1m\033[37mZeroVM\033[0m lightweight VM manager, build 2013-08-04\n"\
    "Usage: <manifest> [-l#] [-v#] [-stFPQ]\n\n"\
    " -l <gigabytes> file size limit (default 4Gb)\n"\
    " -s skip validation\n"\
    " -t report to log instead of stdout\n"\
    " -v <0..3> log verbosity (default 0)\n"\
    " -F quit right before starting user session\n"\
    " -P disable channels space preallocation\n"\
    " -Q disable platform qualification\n"

#define ZEROVM_PRIORITY 19
#define ZEROVM_IO_LIMIT_UNIT 0x40000000l /* 1gb */
#define ZEROVM_IO_LIMIT 4 * ZEROVM_IO_LIMIT_UNIT /* 4gb */
#define LARGEST_NEXE ZEROVM_IO_LIMIT_UNIT

/* validator function from libvalidator.so */
int NaClSegmentValidates(uint8_t* mbase, size_t size, uint32_t vbase);

/*
 * "defense in depth". the last frontier of defense.
 * zerovm limits itself as much as possible
 */
void LastDefenseLine();

/*
 * set limit for the disk writes (in bytes)
 * return 0 if successfully set
 */
int SetStorageLimit(int64_t a);

/* preallocate memory area of given size. abort if fail */
void PreallocateUserMemory(struct NaClApp *nap);

/* serialize system data to user space */
void SetSystemData(struct NaClApp *nap);

EXTERN_C_END

#endif
