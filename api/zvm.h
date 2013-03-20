/*
 * ZeroVM API. contains function prototypes, data structures,
 * macro definitions, types and constants
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
#ifndef ZVM_API_H__
#define ZVM_API_H__ 1

#include <stdint.h>

/* zerovm system calls */
enum TrapCalls
{
  TrapRead = 0x64616552,
  TrapWrite = 0x74697257,
  TrapJail = 0x6c69614a,
  TrapUnjail = 0x6c6a6e55,
  TrapExit = 0x74697845
};

/* channel types */
enum AccessType
{
  SGetSPut, /* sequential read, sequential write */
  RGetSPut, /* random read, sequential write */
  SGetRPut, /* sequential read, random write */
  RGetRPut /* random read, random write */
};

/* channel limits */
enum IOLimits
{
  GetsLimit,
  GetSizeLimit,
  PutsLimit,
  PutSizeLimit,
  IOLimitsCount
};

/* channel descriptor */
struct ZVMChannel
{
  int64_t limits[IOLimitsCount];
  int64_t size; /* 0 for sequential channels */
  enum AccessType type;
  char *name;
};

/* system data available for the user */
struct UserManifest
{
  void *heap_ptr;
  uint32_t heap_size;
  uint32_t stack_size;
  int32_t channels_count;
  struct ZVMChannel *channels;
};

/* pointer to the user manifest */
#define MANIFEST ((const struct UserManifest const *)*((uintptr_t*)0xFEFFFFFC))

/* pointer to trap */
#define TRAP ((int32_t (*)(uint64_t*))0x10000)

/* trap functions */
#define zvm_pread(desc, buffer, size, offset) \
  TRAP((uint64_t[]){TrapRead, 0, desc, (uintptr_t)buffer, size, offset})
#define zvm_pwrite(desc, buffer, size, offset) \
  TRAP((uint64_t[]){TrapWrite, 0, desc, (uintptr_t)buffer, size, offset})
#define zvm_jail(buffer, size) \
  TRAP((uint64_t[]){TrapJail, 0, (uintptr_t)buffer, size})
#define zvm_unjail(buffer, size) \
  TRAP((uint64_t[]){TrapUnjail, 0, (uintptr_t)buffer, size})
#define zvm_exit(code) TRAP((uint64_t[]){TrapExit, 0, code})

#endif /* ZVM_API_H__ */
