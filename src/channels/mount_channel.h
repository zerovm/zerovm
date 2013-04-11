/*
 * channels constructor / destructor API
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

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include <zmq.h>
#include "src/loader/sel_ldr.h"
#include "src/main/etag.h"
#include "api/zvm.h"

EXTERN_C_BEGIN

/* name, id, access type, gets, getsize, puts, putsize */
#define CHANNEL_ATTRIBUTES ChannelAttributesNumber
#define MAX_CHANNELS_NUMBER 6548
#define NET_BUFFER_SIZE 0x10000
#define MOUNTED 1

/* reserved zerovm channels names */
#define RESERVED_CHANNELS 3
#define STDIN "/dev/stdin"
#define STDOUT "/dev/stdout"
#define STDERR "/dev/stderr"

/* attributes has fixed order, thats why enum has been used */
enum ChannelAttributes {
  ChannelName,
  ChannelAlias,
  ChannelAccessType,
  ChannelGets,
  ChannelGetSize,
  ChannelPuts,
  ChannelPutSize,
  ChannelAttributesNumber
};

/* source file types */
enum ChannelSourceType {
  ChannelRegular, /* supported */
  ChannelDirectory, /* not supported */
  ChannelCharacter, /* supported */
  ChannelBlock, /* not supported */
  ChannelFIFO, /* not tested */
  ChannelLink, /* not supported */
  ChannelSocket, /* not supported */
  ChannelIPC, /* to remove */
  ChannelTCP, /* supported */
  ChannelINPROC, /* not supported */
  ChannelPGM, /* not supported */
  ChannelEPGM, /* not supported */
  ChannelUDP, /* going to be supported in the future */
  ChannelSourceTypeNumber
};

/* source file prefixes (should be in synch with ChannelSourceType) */
#define CHANNEL_SOURCE_PREFIXES { \
  "file", /* ChannelRegular */\
  "directory", /* ChannelDirectory */\
  "character", /*ChannelCharacter*/\
  "block", /* ChannelBlock */\
  "fifo", /* ChannelFIFO */\
  "link", /* ChannelLink */ \
  "socket", /* ChannelSocket */\
  "ipc", /* ChannelIPC */\
  "tcp", /* ChannelTCP */\
  "inproc", /* ChannelINPROC */\
  "pgm", /* ChannelPGM */\
  "epgm", /* ChannelEPGM */\
  "udp", /* ChannelUDP */\
  "invalid"\
}

/* zerovm channel descriptor. part of information available for the user side */
struct ChannelDesc
{
  char *name; /* real name */
  char *alias; /* name for untrusted */
  void *tag; /* tag context */
  char digest[TAG_DIGEST_SIZE]; /* tag hexadecimal digest */
  char control[TAG_DIGEST_SIZE]; /* received digest */

  int32_t handle; /* file handle. fit only for regular files */
  void *socket; /* can be used both by network and local channel */

  /* group #2.1 */
  int64_t size; /* channel size */
  /* group #2.2 */
  zmq_msg_t msg; /* 0mq message container. should be initialized */
  int32_t bufpos; /* index of the 1st available byte in the buffer */
  int32_t bufend; /* index of the 1st unavailable byte in the buffer */

  enum AccessType type; /* type of access sequential/random */
  enum ChannelSourceType source; /* network or local file */
  int64_t getpos; /* read position */
  int64_t putpos; /* write position */

  /* limits and counters */
  int64_t limits[IOLimitsCount];
  int64_t counters[IOLimitsCount];

  /* added to serve sequential channels */
  int8_t eof; /* if not 0 the channel reached eof at the last operation */
  int8_t mounted; /* MOUNTED or !MOUNTED */
};

/* construct all channels, initialize it and update system_manifest */
void ChannelsCtor(struct NaClApp *nap);

/* close all channels, update tags */
void ChannelsFinalizer(struct NaClApp *nap);

/* free channels resources */
void ChannelsDtor(struct NaClApp *nap);

/* get string containing protocol name by channel source type */
char *StringizeChannelSourceType(enum ChannelSourceType type);

EXTERN_C_END

#endif /* MOUNT_CHANNEL_H_ */
