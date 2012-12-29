/*
 * name_service.h
 *
 *  Created on: Dec 29, 2012
 *      Author: d'b
 */

#ifndef NAME_SERVICE_H_
#define NAME_SERVICE_H_

#include <glib.h> /* hash table */
#include <byteswap.h> /* for big endian conversion */
#include <arpa/inet.h> /* convert ip <-> int */
#include "src/channels/mount_channel.h"

/*
 * name server expects all data in the big endian. but on the big
 * endian platform name service data doesn't need to be converted
 */
#if BYTE_ORDER == BIG_ENDIAN
#undef bswap_16
#undef bswap_32
#undef bswap_64
#define bswap_16(a)
#define bswap_32(a)
#define bswap_64(a)
#endif

#define PREPOLL_WAIT 1000 /* 1 millisecond */

/*
 * each network channel name consist of 3, ':' delimited parts:
 * 1. protocol ("tcp", "ipc", "gdp", "inproc", "pgm", "epgm")
 * 2. host (real ip or just a number given by manifest)
 * 3. port (NULL, real port or placeholder "0" )
 * some examples:
 * "debug" channel -- tcp:10.11.12.13:44, debug, 0, 0, 0, 999, 999
 * "bind" channel -- tcp:127:0, input.data, 0, 999, 999, 0, 0
 * "connect" channel -- tcp:128: output.data, 0, 0, 0, 999, 999
 */

/*
 * a channel record. contain info to make the binding/connection
 * the full address can be constructed from this record:
 * protocol://host:port
 * note: host can contain packed ip or host number
 */
#define BIND_MARK 0 /* read-only channel */
#define CONNECT_MARK 1 /* write-only channel */
#define OUTSIDER_MARK 2 /* channel not needed name service */

#define LOWEST_AVAILABLE_PORT 49152
#define PARCEL_SIZE 65507 /* maximum size of a UDP packet */
#define PARCEL_NODE_ID 4 /* bytes reserved for the node id */
#define PARCEL_BINDS 4 /* bytes reserved for the binds number */
#define PARCEL_CONNECTS 4 /* bytes reserved for the connects number */
#define NS_PARCEL_JUNK 1 /* ending junk of the name server */
#define PARCEL_REC_SIZE (sizeof(struct ChannelNSRecord)) /* size of the bind/connect record */
#define PARCEL_OVERHEAD (PARCEL_NODE_ID + PARCEL_BINDS + PARCEL_CONNECTS)
#define PARCEL_CAPACITY ((PARCEL_SIZE - PARCEL_OVERHEAD)/sizeof(struct ChannelNSRecord))
#define NS_PARCEL_OVERHEAD PARCEL_CONNECTS + NS_PARCEL_JUNK

/* holds connection information in the HOST order */
struct ChannelConnection
{
  uint8_t protocol; /* to save the memory instead of string number */
  uint32_t host;
  uint16_t port;
  uint8_t mark; /* BIND_MARK, CONNECT_MARK or OUTSIDER_MARK */
};

/*
 * the structure presents needful fields for name server
 * both "bind" and "connect" channels in the parcel use lists
 * containing elements of this structure
 * todo(d'b): to prevent the structure padding the pragma was used
 *   develop some more portable way (casting to array is ugly)
 */
#pragma pack(push, 1)
struct ChannelNSRecord
{
  uint32_t id; /* host id assigned by proxy */
  uint32_t ip; /* host ip assigned by name server */
  uint16_t port; /* host port assigned by zvm or name server */
};
#pragma pack(pop)

/* make url from the given record and return it through the "url" parameter */
void MakeURL(char *url, const int32_t size,
    const struct ChannelDesc *channel, const struct ChannelConnection *record);

/*
 * extract the channel connection information and store it into
 * the netlist hash table. on destruction all allocated memory will
 * be deallocated for key, value and hashtable itself
 */
void StoreChannelConnectionInfo(const struct ChannelDesc *channel);

/* take channel connection info by channel alias */
struct ChannelConnection *GetChannelConnectionInfo(const struct ChannelDesc *channel);

/* poll the name server and update netlist with port information */
void ResolveChannels(struct NaClApp *nap, uint32_t binds, uint32_t connects);

/* initialize name service */
void NameServiceCtor();

/* release name service */
void NameServiceDtor();

/* return 0 if name service is not constructed */
int NameServiceSet();

#endif /* NAME_SERVICE_H_ */
