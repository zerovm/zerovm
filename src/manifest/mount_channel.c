/*
 * channels constructor / destructor
 *
 *  Created on: Dec 5, 2011
 *      Author: d'b
 */
#include <assert.h>
#include <openssl/sha.h>
#include "src/service_runtime/sel_ldr.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/manifest_parser.h"
#include "src/manifest/preload.h"
#include "src/manifest/prefetch.h"
#include "src/manifest/mount_channel.h"

/*
 * returns protocol or ChannelProtoNumber if protocol isn't supported
 * note: can be used globally
 * todo(d'b): use x-macro
 */
enum ChannelNetProtocol GetChannelProtocol(const char *url)
{
  assert(url != NULL);

#define IS_TRANSPORT(prefix, proto) \
  if(strncmp(url, prefix, sizeof(prefix) - 1) == 0) return proto;
  IS_TRANSPORT(IPC_PREFIX, ChannelIPC);
  IS_TRANSPORT(TCP_PREFIX, ChannelTCP);
  IS_TRANSPORT(INPROC_PREFIX, ChannelINPROC);
  IS_TRANSPORT(PGM_PREFIX, ChannelPGM);
  IS_TRANSPORT(EPGM_PREFIX, ChannelEPGM);
  IS_TRANSPORT(UDP_PREFIX, ChannelUDP);
#undef IS_TRANSPORT

  return ChannelProtoNumber;
}

/* get string contain protocol name by protocol id */
/* todo(d'b): use x-macro */
char *StringizeChannelProtocol(enum ChannelNetProtocol id)
{
  switch(id)
  {
    case ChannelIPC: return IPC_PREFIX;
    case ChannelTCP: return TCP_PREFIX;
    case ChannelINPROC: return INPROC_PREFIX;
    case ChannelPGM: return PGM_PREFIX;
    case ChannelEPGM: return EPGM_PREFIX;
    case ChannelUDP: return UDP_PREFIX;
    case ChannelProtoNumber: return NULL; /* to prevent compiler warning */
  }
  return NULL;
}

/*
 * return source type inferred from the channel name. also checks
 * protocol validity for the network channels
 */
static int GetSourceType(char *name)
{
  /* check for design errors */
  assert(name != NULL);

  /* network channel always contain ':'s */
  if(strchr(name, ':') == NULL) return LocalFile;

  COND_ABORT(GetChannelProtocol(name) == ChannelProtoNumber,
      "the specified protocol isn't supported for channels");

  return NetworkChannel;
}

/* return the channel by channel type */
static struct ChannelDesc* SelectNextChannel(struct NaClApp *nap, char *alias)
{
  static int current_channel = RESERVED_CHANNELS;
  struct ChannelDesc *channels;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);
  assert(alias != NULL);

  channels = nap->system_manifest->channels;

  /* check for the standard names */
  if(STREQ(alias, STDIN)) return &channels[STDIN_FILENO];
  if(STREQ(alias, STDOUT)) return &channels[STDOUT_FILENO];
  if(STREQ(alias, STDERR)) return &channels[STDERR_FILENO];

  /* otherwise just return next channel */
  return &channels[current_channel++];
}

/* construct and initialize the channel */
static void ChannelCtor(struct NaClApp *nap, char **tokens)
{
  struct ChannelDesc *channel;

  assert(nap != NULL);
  assert(tokens != NULL);
  assert(nap->system_manifest != NULL);
  assert(nap->system_manifest->channels != NULL);

  /*
   * pick the channel and check if the channel is available
   * channels must not have duplicate aliases
   */
  channel = SelectNextChannel(nap, tokens[ChannelAlias]);
  COND_ABORT(channel->alias != NULL, "channel is already allocated");

  channel->type = ATOI(tokens[ChannelAccessType]);
  COND_ABORT(channel->type < SGetSPut || channel->type > RGetRPut,
      "invalid channel access type");
  channel->name = tokens[ChannelName];
  channel->alias = tokens[ChannelAlias];
  COND_ABORT(SHA1_Init(&channel->tag) == 0, "channel tag setup error");
  channel->source = GetSourceType((char*)channel->name);

  /* limits and counters */
  channel->limits[GetsLimit] = ATOI(tokens[ChannelGets]);
  channel->limits[GetSizeLimit] = ATOI(tokens[ChannelGetSize]);
  channel->limits[PutsLimit] = ATOI(tokens[ChannelPuts]);
  channel->limits[PutSizeLimit] = ATOI(tokens[ChannelPutSize]);
  channel->counters[GetsLimit] = 0;
  channel->counters[GetSizeLimit] = 0;
  channel->counters[PutsLimit] = 0;
  channel->counters[PutSizeLimit] = 0;
  channel->closed = 0;

  /* mount given channel */
  switch(channel->source)
  {
    int code;
    case LocalFile:
      code = PreloadChannelCtor(channel);
      COND_ABORT(code, "cannot allocate local file channel");
      break;
    case NetworkChannel:
      code = PrefetchChannelCtor(channel);
      COND_ABORT(code, "cannot allocate network channel");
      break;
    default:
      NaClLog(LOG_FATAL, "invalid channel source type\n");
      break;
  }
}

/*
 * accumulates hashes of all channels into the one digest
 * returns given string if updated or NULL when failed.
 * if NULL is given finalizes hash and return digest
 */
static unsigned char *UpdateChannelsHash(const unsigned char *hash)
{
  static unsigned char ghash[SHA_DIGEST_LENGTH];
  static SHA_CTX ctx;
  static int start = 1;
  int i;

  /* in the 1st time call initialize sha-1 context */
  if(start)
  {
    SHA1_Init(&ctx);
    start = 0;
  }

  /* if NULL specified finalize an set etag in system manifest */
  if(hash == NULL)
  {
    SHA1_Final(ghash, &ctx);
    return ghash;
  }

  // update hash with the new value
  i = SHA1_Update(&ctx, hash, SHA_DIGEST_LENGTH);
  ErrIf(i == 0, "error updating etag");
  return i == 0 ? NULL : (unsigned char*)hash;
}

/* close channel and deallocate its resources */
static void ChannelDtor(struct ChannelDesc *channel)
{
  unsigned char hash[SHA_DIGEST_LENGTH];
  int i;

  assert(channel != NULL);

  /*
   * update the global channel hash value
   * note: if there are unsent/unread messages they will be lost
   *   and not taken into account
   */
  i = SHA1_Final(hash, &channel->tag);
  ErrIf(i != 1, "channel '%s' tag finalization error", channel->alias);
  UpdateChannelsHash(hash);

  switch(channel->source)
  {
    case LocalFile:
      PreloadChannelDtor(channel);
      break;
    case NetworkChannel:
      PrefetchChannelDtor(channel);
      break;
    default:
      NaClLog(LOG_ERROR, "unknown channel source\n");
      break;
  }
}

/*
 * construct and initialize all channels
 * note: standard channels must be specified in manifest
 */
void ChannelsCtor(struct NaClApp *nap)
{
  int i;
  char *values[MAX_CHANNELS_NUMBER];
  struct SystemManifest *mft;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /*
   * calculate channels count. maximum allowed (MAX_CHANNELS_NUMBER - 1)
   * channels, minimum - RESERVED_CHANNELS
   */
  mft = nap->system_manifest;
  mft->channels_count =
      GetValuesByKey("Channel", values, MAX_CHANNELS_NUMBER);
  COND_ABORT(mft->channels_count >= MAX_CHANNELS_NUMBER,
      "channels number reached maximum");
  COND_ABORT(mft->channels_count < RESERVED_CHANNELS,
      "not all standard channels are provided");

  /* allocate memory for channels */
  mft->channels = calloc(mft->channels_count, sizeof(*mft->channels));
  COND_ABORT(mft->channels == NULL, "cannot allocate memory for channels");

  /* parse channels. 0..2 reserved for stdin/stdout/stderr */
  for(i = 0; i < mft->channels_count; ++i)
  {
    char *tokens[CHANNEL_ATTRIBUTES + 1]; /* to detect wrong attributes number */
    int count = ParseValue(values[i], ", \t", tokens, CHANNEL_ATTRIBUTES + 1);
    COND_ABORT(count != CHANNEL_ATTRIBUTES, "wrong number of the channel attributes");

    /* construct and initialize channel */
    ChannelCtor(nap, tokens);
  }

  /* channels array validation */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    COND_ABORT(nap->system_manifest->channels[i].name == NULL,
               "the channels array must not have uninitialized elements");
  COND_ABORT(nap->system_manifest->channels_count < RESERVED_CHANNELS,
      "some of the standard channels weren't initialized");

  /* 2nd pass for the network channels if name service specified */
  KickPrefetchChannels(nap);
}

/* finalize all channels and free memory */
void ChannelsDtor(struct NaClApp *nap)
{
  int i;

  /* exit if channels are not constructed */
  assert(nap != NULL);
  if(nap->system_manifest == NULL) return;
  if(nap->system_manifest->channels == NULL) return;
  if(nap->system_manifest->channels_count == 0) return;

  for(i = 0; i < nap->system_manifest->channels_count; ++i)
    ChannelDtor(&nap->system_manifest->channels[i]);

  free(nap->system_manifest->channels);

  /* update etag */
  nap->system_manifest->etag = UpdateChannelsHash(NULL);
}
