/*
 * mount_channel.h
 *
 *  Created on: Dec 6, 2011
 *      Author: dazo
 */

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include <openssl/sha.h> /* SHA_DIGEST_LENGTH, SHA_CTX */
#include <zmq.h>
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/etag.h"
#include "api/zvm.h"

EXTERN_C_BEGIN

#define MAX_CHANNELS_NUMBER 6548

/* name, id, access type, gets, getsize, puts, putsize */
#define CHANNEL_ATTRIBUTES ChannelAttributesNumber

/* stdin, stdout, stderr. in the future 2 more channels will be added */
#define RESERVED_CHANNELS 3
#define NET_BUFFER_SIZE 0x10000

/* allowed network prefixes */
#define IPC_PREFIX "ipc"
#define TCP_PREFIX "tcp"
#define INPROC_PREFIX "inproc"
#define PGM_PREFIX "pgm"
#define EPGM_PREFIX "epgm"
#define UDP_PREFIX "udp"

/* list of protocols supported for zvm channels */
enum ChannelNetProtocol
{
  ChannelIPC, /* not fully: socket should already exist */
  ChannelTCP,
  ChannelINPROC, /* not tested */
  ChannelPGM, /* not tested */
  ChannelEPGM, /* not tested */
  ChannelUDP, /* going to be supported in the future */
  ChannelProtoNumber /* number of supported protocols yet means protocol isn't supported */
};

/* reserved zerovm channels names */
#define STDIN "/dev/stdin" /* c90 stdin */
#define STDOUT "/dev/stdout" /* c90 stdout */
#define STDERR "/dev/stderr" /* c90 stderr */
#define STDDBG "/dev/debug" /* zvm extension */

/* not used so far */
#define INPUT "/dev/input" /* random access read-only channel */
#define OUTPUT "/dev/input" /* random access write-only channel */

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

enum ChannelSourceType {
  LocalFile,
  NetworkChannel,
  ChannelSourceTypeNumber
};

/*
 * zerovm channel descriptor. part of information available for the user side
 * todo(d'b): replace "closed" with "flags" field. will contain EOF, errors, e.t.c.
 */
struct ChannelDesc
{
  char *name; /* real name */
  char *alias; /* name for untrusted */
  char tag[TAG_CONTEXT_SIZE]; /* tag context (currently sha1) */
  char digest[TAG_DIGEST_SIZE]; /* tag digest (currently sha1) */

  /* group #1.1 */
  int32_t handle; /* file handle */
  /* group #1.2 */
  void *socket; /* zmq socket */

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
};

/* construct all channels, initialize it and update system_manifest */
void ChannelsCtor(struct NaClApp *nap);

/* close all channels, initialize it and update system_manifest */
void ChannelsDtor(struct NaClApp *nap);

/* returns protocol or ChannelProtoNumber if protocol isn't supported */
enum ChannelNetProtocol GetChannelProtocol(const char *url);

/* get string with the protocol name by protocol id */
char *StringizeChannelProtocol(enum ChannelNetProtocol id);

EXTERN_C_END

#endif /* MOUNT_CHANNEL_H_ */
