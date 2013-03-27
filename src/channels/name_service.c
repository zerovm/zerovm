/*
 * name service for network channels
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

#include <assert.h>
#include <errno.h>
#include "src/main/tools.h"
#include "src/main/zlog.h"
#include "src/main/manifest_parser.h"
#include "src/main/manifest_setup.h"
#include "src/channels/name_service.h"

#include "src/channels/prefetch.h" /* for GetChannelProtocol() */

/*
 * hashtable to hold connection information about all network channels
 * will be used even if name server isn't specified because network channels
 * has special url format which cannot be used without translation:
 * proto:host:port (instead of standard "proto://host:port")
 */
static GHashTable *netlist = NULL;
static void *connects_index = NULL; /* points to the "connect" list start */
static uint32_t binds = 0; /* "bind" channel number */
static uint32_t connects = 0; /* "connect" channel number */
static struct ChannelConnection *nameservice = NULL;

/* test the channel for validity */
static void FailOnInvalidNetChannel(const struct ChannelDesc *channel)
{
  ZLOGFAIL(channel->source != ChannelTCP, EPROTONOSUPPORT,
      "%s has invalid protocol %s", channel->alias, channel->source);
  ZLOGFAIL(channel->type != SGetSPut, EFAULT,
      "%s is a network channel and must be sequential", channel->alias);
  ZLOGFAIL(channel->limits[GetsLimit] && channel->limits[PutsLimit] != 0,
      EFAULT, "%s is a network channel and must be read-only or write-only",
      channel->alias);
}

/*
 * create the 32-bit netlist key from the ChannelConnection record
 * using channel host id and channel mark (bind, connect or outsider)
 * note: even for channels marked as outsiders a key will be created
 */
static INLINE uint32_t MakeKey(const struct ChannelConnection *record)
{
  uint32_t result;

  assert(record != NULL);

  result = ((uint32_t)record->mark)<<24 ^ record->host;
  ZLOG(LOG_INSANE, "mark = %u, host = %u, result = %u",
      record->mark, record->host, result);
  return result;
}

/* convert ip address (or node id) to integer */
static uint32_t ExtractHost(const char *strip)
{
  uint32_t result;

  if(strchr(strip, '.') != NULL)
  {
    struct sockaddr_in sa;
    result = inet_pton(AF_INET, strip, &sa.sin_addr);
    result = result == 1 ? bswap_32(sa.sin_addr.s_addr) : 0;
  }
  else
    result = ATOI(strip);

  return result;
}

/*
 * parse the given channel name and initialize the record for the netlist table
 * return 0 if proper url found in the given channel and parsed without errors
 */
static void ParseURL(const struct ChannelDesc *channel,
    struct ChannelConnection *record)
{
  char *buf[BIG_ENOUGH_STRING], **tokens = buf;
  char name[BIG_ENOUGH_STRING];

  assert(channel != NULL);
  assert(channel->name != NULL);
  assert(record != NULL);
  assert(channel->source == ChannelTCP);

  ZLOGS(LOG_INSANE, "for %s", channel->alias);

  /* copy the channel name aside and parse it */
  g_strlcpy(name, channel->name, BIG_ENOUGH_STRING);
  ParseValue(name, ":", tokens, BIG_ENOUGH_STRING);
  assert(tokens[0] != NULL);

  /*
   * initiate the record. there are 3 cases, host is:
   * 1. a host number like "123" port isn't specified
   * 2. an ip address like "1.2.3.4" port specified
   * 3. named host like "dark.place.gov" port specified
   * note: only 1st case needs name service
   * note: case 3 needs special MakeURL() branch. so far isn't supported
   */
  record->protocol = GetChannelProtocol(tokens[0]);
  record->port = ATOI(tokens[2]);
  record->host = ExtractHost(tokens[1]);
  ZLOGFAIL(record->host == 0, ENXIO,
      "%s has invalid url %s", channel->alias, channel->name);

  /* mark the channel as "bind", "connect" or "outsider" */
  if(record->port != 0) record->mark = OUTSIDER_MARK;
  else record->mark = channel->limits[GetsLimit]
      && channel->limits[GetSizeLimit] ? BIND_MARK : CONNECT_MARK;
}

void StoreChannelConnectionInfo(const struct ChannelDesc *channel)
{
  struct ChannelConnection *record;

  assert(channel != NULL);

  /* initiate the hash table if not yet */
  record = g_malloc(sizeof *record);

  /* prepare and store the channel connection record */
  ZLOGS(LOG_DEBUG, "validating %s", channel->alias);
  FailOnInvalidNetChannel(channel);
  ParseURL(channel, record);
  g_hash_table_insert(netlist, GUINT_TO_POINTER(MakeKey(record)), record);
}

struct ChannelConnection *GetChannelConnectionInfo(const struct ChannelDesc *channel)
{
  struct ChannelConnection r; /* fake record */

  assert(channel != NULL);
  assert(netlist != NULL);

  ParseURL(channel, &r);
  return (struct ChannelConnection*)
      g_hash_table_lookup(netlist, GUINT_TO_POINTER(MakeKey(&r)));
}

/*
 * "binds" and "connects" records serializer. used to build
 * parcel for the name service. expects "binds" and "connects"
 * describes real counts and given "buffer" have enough space
 * for the data. convert all values to BIG ENDIAN (if needed).
 * down count to 0 local variables: "binds" and "connects"
 * note: the function uses 3 local variables: binds, connects, connects_index
 */
static void NSRecordSerializer(gpointer key, gpointer value, gpointer buffer)
{
  struct ChannelConnection *record;
  struct ChannelNSRecord *p = buffer;

  assert((uint32_t)(uintptr_t)key != 0);
  assert(value != NULL);
  assert(buffer != NULL);

  /* get channel connection information */
  record = (struct ChannelConnection*)value;
  if(record->mark == OUTSIDER_MARK) return;

  /* detect the channel type (bind or connect) and update the counter */
  p = (void*)(record->mark == BIND_MARK ?
      (char*)buffer + --binds * sizeof(struct ChannelNSRecord) :
      (char*)connects_index + --connects * sizeof(struct ChannelNSRecord));

  /* store channel information into the buffer */
  p->id = bswap_32(record->host);
  p->ip = 0;
  p->port = bswap_16(record->port);
}

/*
 * create parcel (in provided place) for the name server
 * return the real parcel size. bswap_32 will work only on big endian
 */
static int32_t ParcelCtor(const struct NaClApp *nap,
    char *parcel, uint32_t size)
{
  char *p = parcel; /* parcel pointer */
  uint32_t node_id_network = bswap_32(nap->system_manifest->node_id);
  uint32_t binds_network = bswap_32(binds);
  uint32_t connects_network = bswap_32(connects);

  /* asserts and checks */
  assert(parcel != NULL);
  assert(size >= connects * sizeof(struct ChannelNSRecord) + PARCEL_OVERHEAD);

  /*
   * iterate through netlist. will update "binds" and "connects"
   * the parcel will be updated with "bind" and "connect" lists
   */
  connects_index = parcel + PARCEL_NODE_ID + PARCEL_BINDS
      + binds * PARCEL_REC_SIZE + PARCEL_CONNECTS;
  g_hash_table_foreach(netlist, NSRecordSerializer,
      parcel + PARCEL_NODE_ID + PARCEL_BINDS);
  assert(binds + connects == 0);

  /* restore "binds" and "connects" */
  binds = bswap_32(binds_network);
  connects = bswap_32(connects_network);

  /* put the node id into the parcel */
  memcpy(p, &node_id_network, sizeof node_id_network);

  /* put the the "binds counter" into the parcel */
  p += PARCEL_NODE_ID;
  memcpy(p, &binds_network, PARCEL_BINDS);

  /* put the the "connects counter" into the parcel */
  p += PARCEL_BINDS + PARCEL_REC_SIZE * binds;
  memcpy(p, &connects_network, PARCEL_CONNECTS);

  /* return the actual parcel size */
  return PARCEL_OVERHEAD + PARCEL_REC_SIZE * binds +  PARCEL_REC_SIZE * connects;
}

/*
 * send the parcel to the name server and get the answer
 * return the actual parcel size
 * note: given parcel variable will be updated with the ns answer
 */
static int32_t SendParcel(char *parcel, uint32_t size)
{
  int ns_socket;
  int32_t result;
  struct sockaddr_in ns;
  uint32_t ns_len = sizeof ns;

  assert(parcel != NULL);
  assert(size > 0);

  /* connect to the name server */
  ns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  ns.sin_addr.s_addr = bswap_32(nameservice->host);
  ns.sin_port = bswap_16(nameservice->port);
  ns.sin_family = AF_INET;

  /* send the parcel to the name server */
  result = sendto(ns_socket, parcel, size, 0, (void*)&ns, sizeof ns);
  ZLOGFAIL(result == -1, errno, "failed to send parcel to the name server");

  /* receive the parcel back */
  result = recvfrom(ns_socket, parcel, size, 0, (void*)&ns, &ns_len);
  ZLOGFAIL(result == -1, errno, "failed to receive parcel from the name server");
  close(ns_socket);

  return result;
}

/* decode the parcel and update netlist */
static void DecodeParcel(const char *parcel, uint32_t count)
{
  uint32_t i;
  const struct ChannelNSRecord *records;
  uint32_t size = (count - NS_PARCEL_OVERHEAD) / PARCEL_REC_SIZE;

  assert(parcel != NULL);
  assert(bswap_32(*(uint32_t*)parcel) == size);
  if(size == 0) return;

  /* take a record by record from the parcel and update netlist */
  records = (void*)(parcel + PARCEL_CONNECTS);
  for(i = 0; i < size; ++i)
  {
    struct ChannelConnection r, *record = &r;

    /* get the connection record from netlist */
    record->host = bswap_32(records[i].id);
    record->mark = CONNECT_MARK;
    record = g_hash_table_lookup(netlist, GUINT_TO_POINTER(MakeKey(record)));
    assert(record != NULL);

    /* update the record */
    record->port = bswap_16(records[i].port);
    record->host = bswap_32(records[i].ip);
    assert(record->host != 0);
    assert(record->port != 0);
  }
}

void ResolveChannels(const struct NaClApp *nap, uint32_t all_binds, uint32_t all_connects)
{
  char parcel[PARCEL_SIZE];
  int32_t parcel_size = PARCEL_SIZE;

  binds = all_binds;
  connects = all_connects;

  assert(binds + connects <= PARCEL_CAPACITY);
  assert(nap != NULL);

  /* construct the parcel from netlist */
  parcel_size = ParcelCtor(nap, parcel, parcel_size);
  if(parcel_size == 0) return;

  /* send it to name server and get the answer */
  parcel_size = SendParcel(parcel, parcel_size);
  assert(parcel_size == NS_PARCEL_OVERHEAD + connects * PARCEL_REC_SIZE);

  /* decode the parcel and update netlist */
  DecodeParcel(parcel, parcel_size);
}

void NameServiceCtor()
{
  struct ChannelDesc channel;

  netlist = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
  ZLOGFAIL(netlist == NULL, EFAULT, "cannot allocate netlist");

  /* get name service connection string if available */
  memset(&channel, 0, sizeof channel);
  channel.source = ChannelTCP;
  channel.alias = MFT_NAMESERVER;
  channel.name = GetValueByKey(MFT_NAMESERVER);

  /* parse the given string and make url (static var) */
  if(channel.name != NULL)
  {
    nameservice = g_malloc(sizeof *nameservice);
    ParseURL(&channel, nameservice);
    ZLOGFAIL(nameservice->mark != OUTSIDER_MARK, EFAULT, "invalid name server type");
    ZLOGFAIL(nameservice->host == 0, EFAULT, "invalid name server host");
    ZLOGFAIL(nameservice->port == 0, EFAULT, "invalid name server port");
    ZLOGFAIL(nameservice->protocol != ChannelUDP, EPROTONOSUPPORT,
        "only udp supported for name server");
  }
}

void NameServiceDtor()
{
  g_hash_table_destroy(netlist);
  g_free(nameservice);
}

int NameServiceSet()
{
  return nameservice != NULL;
}
