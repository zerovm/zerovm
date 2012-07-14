/*
 * mount_channel.h
 *
 *  Created on: Dec 6, 2011
 *      Author: dazo
 */

#ifndef MOUNT_CHANNEL_H_
#define MOUNT_CHANNEL_H_

#include "src/service_runtime/sel_ldr.h"
#include "api/zvm.h"

EXTERN_C_BEGIN

#define MAX_CHANNELS_NUMBER BIG_ENOUGH_SPACE
#define CHANNEL_ATTRIBUTES ChannelAttributesNumber /* name, id, access type, gets, getsize, puts, putsize */
#define RESERVED_CHANNELS 3 /* stdin, stdout, stderr */

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

/* zerovm channel descriptor. part of information available for the user side */
struct ChannelDesc
{
  char *name; /* file name */
  int32_t handle; /* file handle */
  enum AccessType type; /* type of access sequential/random */
  int64_t size; /* channel size */
  int64_t getpos; /* read position */
  int64_t putpos; /* write position */

  /* limits and counters */
  int64_t limits[IOLimitsCount];
  int64_t counters[IOLimitsCount];
};

/* construct all channels, initialize it and update system_manifest */
void ChannelsCtor(struct NaClApp *nap);

/* close all channels, initialize it and update system_manifest */
void ChannelsDtor(struct NaClApp *nap);

EXTERN_C_END

#endif /* MOUNT_CHANNEL_H_ */
