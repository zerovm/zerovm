/*
 * mount_channel.h
 *
 *  Created on: Dec 6, 2011
 *      Author: dazo
 */

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include <zmq.h>
#include "src/service_runtime/sel_ldr.h"
#include "api/zvm.h"

EXTERN_C_BEGIN

#define MAX_CHANNELS_NUMBER BIG_ENOUGH_SPACE

/* name, id, access type, gets, getsize, puts, putsize */
#define CHANNEL_ATTRIBUTES ChannelAttributesNumber

/* we just need unique id to pass eof state to untrusted */
#define ZVM_EOF -0x2012

/* stdin, stdout, stderr. in the future 2 more channels will be added */
#define RESERVED_CHANNELS 3
#define NET_BUFFER_SIZE 0x10000

/* allowed network prefixes */
#define IPC_PREFIX "ipc://"
#define TCP_PREFIX "tcp://"
#define INPROC_PREFIX "inproc://"
#define PGM_PREFIX "pgm://"
#define EPGM_PREFIX "epgm://"

/* reserved zerovm channels names */
#define STDIN "/dev/stdin" /* c90 stdin */
#define STDOUT "/dev/stdout" /* c90 stdout */
#define STDERR "/dev/stderr" /* c90 stderr */

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
 * todo(d'b): add "flags" field. will contain EOF, i/o errors, e.t.c.
 */
struct ChannelDesc
{
  char *name; /* real name */
  char *alias; /* name for untrusted */

  /* group #1.1 */
  int32_t handle; /* file handle */
  /* group #1.2 */
  void *socket; /* zmq socket */

  /* group #2.1 */
  int64_t size; /* channel size */
  /* group #2.2 */
  char *buffer; /* fixed size buffer to hold the message leftovers */
  int32_t bufpos; /* index of the 1st available byte in the buffer */
  int32_t bufend; /* index of the 1st unavailable byte in the buffer */
  zmq_msg_t *msg; /* zeromq message container. should be initialized before usage */

  enum AccessType type; /* type of access sequential/random */
  enum ChannelSourceType source; /* network or local file */
  int64_t getpos; /* read position */
  int64_t putpos; /* write position */

  /* limits and counters */
  int64_t limits[IOLimitsCount];
  int64_t counters[IOLimitsCount];
  int8_t closed; /* if not 0 the channel is opened */
};

/* construct all channels, initialize it and update system_manifest */
void ChannelsCtor(struct NaClApp *nap);

/* close all channels, initialize it and update system_manifest */
void ChannelsDtor(struct NaClApp *nap);

EXTERN_C_END

#endif /* MOUNT_CHANNEL_H_ */
