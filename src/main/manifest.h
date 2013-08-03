/*
 * manifest_parse.h
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

#ifndef MANIFEST_H__
#define MANIFEST_H__ 1

#include "api/zvm.h"
#include "src/loader/sel_ldr.h"

EXTERN_C_BEGIN

/* x-macro helpers */
#define XTYPE(a) enum ENUM_##a
#define XSIZE(a) (sizeof ARRAY_##a / sizeof *ARRAY_##a)
#define XSTR(a, c) ARRAY_##a[c]

/* (x-macro): channels protocols */
#define PROTOCOLS \
    X(TCP) \
    X(UDP) \
    X(IPC) \
    X(INPROC) \
    X(PGM) \
    X(EPGM) \
    X(Regular) \
    X(Directory) \
    X(Character) \
    X(Block) \
    X(FIFO) \
    X(Link) \
    X(Socket)

/* (x-macro): manifest enumeration and array */
#define XENUM(a) enum ENUM_##a {a};
#define X(a) Proto ## a,
  XENUM(PROTOCOLS)
#undef X

/* network channel description */
struct Connection {
  uint8_t protocol; /* XTYPE(PROTOCOLS) */
  void *handle; /* pointer to (0mq) socket */
  uint16_t port;
  uint32_t host;
  uint8_t flags; /* bits: 0 - host id/ip, 1 - r/w */
};

/* local channel description */
struct File {
  uint8_t protocol; /* XTYPE(PROTOCOLS) */
  void *handle; /* (int*) or (FILE*) */
  char *name;
};

/* channel structure */
struct ChannelDesc {
  /* manifest parser initialize it (partially) */
  GPtrArray *source; /* (Connection*) or (File*) */
  char *alias; /* name for untrusted */
  enum ChannelType type; /* type of access sequential/random */
  void *tag; /* tag context */
  int64_t limits[LimitsNumber];
  int8_t eof;

  /* constructor initialize it */
  void *msg; /* network message container */
  int64_t size; /* file size (or 0) */
  int64_t getpos; /* source read position */
  int64_t putpos; /* source write position */
  int32_t bufpos; /* index of the 1st available byte in the buffer */
  int32_t bufend; /* index of the 1st unavailable byte in the buffer */
  int64_t counters[LimitsNumber];
};

/* zerovm manifest structure */
struct Manifest {
  int node; /* own node id from manifest */
  char *program; /* program file name */
  char *etag; /* signature. reserved for a future */
  int32_t timeout; /* time user module allowed to run */
  int64_t mem_size; /* user specified memory */
  void *mem_tag; /* tag context */
  struct Connection *name_server;
  GPtrArray *channels; /* all elements are (ChannelDesc*) */
};

/* de-serialize manifest from the given file */
struct Manifest *ManifestCtor(const char *name);

/*
 * release manifest resources. all elements initialized by another classes
 * must be deallocated by those classes
 */
void ManifestDtor(struct Manifest *manifest);

/*
 * serialize manifest to user space. the target address
 * and size isn't known but it is known that the data should
 * be placed right before the user stack. therefore the function
 * returns the pointer and size of created user manifest
 * todo: implement it
 */
void UserManifest(struct Manifest *manifest);

/*
 * serialize to snapshot
 * todo: implement it
 */
void Freeze(struct Manifest *manifest);

/*
 * de-serialize from snapshot
 * todo: implement it
 */
void UnFreeze(struct Manifest *manifest);
EXTERN_C_END

#endif
