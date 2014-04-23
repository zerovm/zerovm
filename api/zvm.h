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
  TrapTest = 0x74736554, /* TODO(d'b): should be removed before release */

  TrapRead = 0x64616552,
  TrapWrite = 0x74697257,
  TrapProt = 0x746f7250,
  TrapFork = 0x6b726f46,
  TrapExit = 0x74697845
};

/* channel types */
enum ChannelType {
  SGetSPut, /* sequential read, sequential write */
  RGetSPut, /* random read, sequential write */
  SGetRPut, /* sequential read, random write */
  RGetRPut /* random read, random write */
};

/* channel limits */
enum ChannelLimits {
  GetsLimit,
  GetSizeLimit,
  PutsLimit,
  PutSizeLimit,
  LimitsNumber
};

/* channel descriptor */
struct ZVMChannel
{
  int64_t limits[LimitsNumber];
  int64_t size; /* 0 for sequential channels */
  enum ChannelType type;
  char *name;
};

/* system data available for the user */
struct UserManifest
{
  void *heap_ptr;
  uint32_t heap_size;
  uint32_t stack_size;
  int32_t channels_count;
  struct ZVMChannel channels[0];
};

/* pointer to the user manifest (read only memory area) */
#define MANIFEST ((const struct UserManifest const*)(uintptr_t)0xFF000000)

/* trap pointer. internal helper. DO NOT use it! */
#define TRAP ((int32_t (*)(uint64_t*))0x10000)

/*
 * trap functions
 *
 * zvm_pread
 *   read from "offset" position of "desc" channel "size" bytes to "buffer"
 * zvm_pwrite
 *   write to "offset" position of "desc" channel "size" bytes from "buffer"
 * zvm_mprotect
 *   protect "size" bytes from "buffer" with "prot"
 * zvm_fork
 *   ask for fork (for further details see "daemon mode")
 * zvm_exit
 *   terminate program with "code"
 *
 * all trap functions (except zvm_fork) has same return as libc ones, but
 * instead of setting errno and returning -1 it returns -errno
 *
 * note: zvm_jail, zvm_unjail removed, but can be emulated with zvm_mprotect:
 * zvm_jail(buffer, size) => zvm_mprotect(buffer, size, PROT_READ | PROT_EXEC)
 * zvm_unjail(buffer, size) => zvm_mprotect(buffer, size, PROT_READ | PROT_WRITE)
 */
#define zvm_pread(desc, buffer, size, offset) \
  TRAP((uint64_t[]){TrapRead, 0, desc, (uintptr_t)buffer, size, offset})
#define zvm_pwrite(desc, buffer, size, offset) \
  TRAP((uint64_t[]){TrapWrite, 0, desc, (uintptr_t)buffer, size, offset})
#define zvm_mprotect(buffer, size, prot) \
  TRAP((uint64_t[]){TrapProt, 0, (uintptr_t)buffer, size, prot})
#define zvm_fork() TRAP((uint64_t[]){TrapFork})
#define zvm_exit(code) TRAP((uint64_t[]){TrapExit, 0, code})

#endif /* ZVM_API_H__ */
