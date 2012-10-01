/*
 * preallocate network channel
 * note: made over zeromq library
 *
 * todo(d'b): refactor the code. extract name service to own file.
 *   MakeUrl() should assume that the proper url has been passed and
 *   abort if it is not (make appropriate changes in dependant functions)
 *
 * based on code created by YaroslavLitvinov
 * updated on: Dec 5, 2011
 *     Author: d'b
 */
#include <assert.h>
#include <glib.h> /* hash table */
#include <byteswap.h> /* for big endian conversion */
#include <arpa/inet.h> /* convert ip <-> int */
#include <zmq.h>
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h" /* todo(d'b): remove it. defines SystemManifest */
#include "src/manifest/prefetch.h"
#include "src/service_runtime/etag.h" /* ETAG_SIZE. corruption control */
#include "src/service_runtime/nacl_globals.h" /* gnap */

static uint32_t channels_cnt = 0; /* needs for NetCtor/Dtor */
static void *context = NULL; /* zeromq context */

/*
 * todo(d'b): move long comments describing channels design to the documentation
 * todo(d'b): move to own file {{
 */

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

/*
 * hashtable to hold connection information about all network channels
 * will be used even if name server isn't specified because network channels
 * has special url format which cannot be used without translation:
 * proto:host:port (instead of standard "proto://host:port")
 */
static GHashTable *netlist = NULL;
static uint32_t binds = 0; /* "bind" channel number */
static uint32_t connects = 0; /* "connect" channel number */
static void *connects_index = NULL; /* points to the "connect" list start */
static struct ChannelConnection *nameservice = NULL;

/*
 * test the channel for validity
 * todo(d'b): update the function with more checks or remove it
 */
static void FailOnInvalidNetChannel(const struct ChannelDesc *channel)
{
  COND_ABORT(channel->source != NetworkChannel, "channel isn't NetworkChannel");
  COND_ABORT(channel->type != SGetSPut, "network channel must be sequential");
  COND_ABORT(channel->limits[GetsLimit] && channel->limits[PutsLimit] != 0,
      "network channel must be read-only or write-only");
}

/*
 * create the 32-bit netlist key from the ChannelConnection record
 * using channel host id and channel mark (bind, connect or outsider)
 * note: even for channels marked as outsiders a key will be created
 */
inline static uint32_t MakeKey(const struct ChannelConnection *record)
{
  uint32_t result;
  NaClLog(LOG_SUICIDE, "%s; %s, %d: ", __FILE__, __func__, __LINE__);
  assert(record != NULL);

  result = ((uint32_t)record->mark)<<24 ^ record->host;
  NaClLog(LOG_SUICIDE, "%s; %s, %d: mark = %u, host = %u",
      __FILE__, __func__, __LINE__, record->mark, record->host);
  NaClLog(LOG_SUICIDE, "%s; %s, %d: result = %u", __FILE__, __func__, __LINE__, result);
  return result;
}

/*
 * parse the given channel name and initialize the record for the netlist table
 * return 0 if proper url found in the given channel and parsed without errors
 */
static void ParseURL(const struct ChannelDesc *channel, struct ChannelConnection *record)
{
  char *buf[BIG_ENOUGH_SPACE], **tokens = buf;
  char name[BIG_ENOUGH_SPACE];

  assert(channel != NULL);
  assert(channel->name != NULL);
  assert(record != NULL);
  assert(channel->source == NetworkChannel);

  NaClLog(LOG_SUICIDE, "%s; %s, %d: url = %s", __FILE__, __func__, __LINE__, channel->name);

  /* copy the channel name aside and parse it */
  strncpy(name, channel->name, BIG_ENOUGH_SPACE);
  name[BIG_ENOUGH_SPACE - 1] = '\0';
  ParseValue(name, ":", tokens, BIG_ENOUGH_SPACE);
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
  assert(record->protocol != ChannelProtoNumber);

  record->port = ATOI(tokens[2]);
  record->host = record->port == 0 ? ATOI(tokens[1]) : bswap_32(inet_addr(tokens[1]));
  COND_ABORT(record->host == 0 && record->port == 0, "invalid channel url");

  /* mark the channel as "bind", "connect" or "outsider" */
  if(record->port != 0) record->mark = OUTSIDER_MARK;
  else record->mark = channel->limits[GetsLimit]
      && channel->limits[GetSizeLimit] ? BIND_MARK : CONNECT_MARK;
}

/* make url from the given record and return it through the "url" parameter */
static void MakeURL(char *url, const int32_t size,
    const struct ChannelDesc *channel, const struct ChannelConnection *record)
{
  char host[BIG_ENOUGH_SPACE];

  assert(url != NULL);
  assert(record != NULL);
  assert(channel != NULL);
  assert(channel->name != NULL);

  COND_ABORT(record->host == 0 && record->port != 0,
      "named host isn't supporting yet");
  COND_ABORT(size < 6, "too short url buffer");

  /* create string containing ip or id as the host name */
  switch(record->mark)
  {
    struct in_addr ip;
    case BIND_MARK:
      snprintf(host, BIG_ENOUGH_SPACE, "*");
      break;

    case CONNECT_MARK:
    case OUTSIDER_MARK:
      ip.s_addr = bswap_32(record->host);
      snprintf(host, BIG_ENOUGH_SPACE, "%s", inet_ntoa(ip));
      break;

    default:
      COND_ABORT(1, "unknown channel mark");
      break;
  }

  /* construct url */
  host[BIG_ENOUGH_SPACE - 1] = '\0';
  snprintf(url, size, "%s://%s:%u",
      StringizeChannelProtocol(record->protocol), host, record->port);
  url[size-1] = '\0';

  NaClLog(LOG_SUICIDE, "%s; %s, %d: url = %s", __FILE__, __func__, __LINE__, url);
}

/*
 * extract the channel connection information and store it into
 * the netlist hash table. on destruction all allocated memory will
 * be deallocated for key, value and hashtable itself
 */
static void StoreChannelConnectionInfo(const struct ChannelDesc *channel)
{
  struct ChannelConnection *record;

  assert(channel != NULL);

  /* initiate the hash table if not yet */
  record = malloc(sizeof *record);
  COND_ABORT(record == NULL, "cannot allocate memory to hold connection info");

  /* prepare and store the channel connection record */
  NaClLog(LOG_INFO, "validating channel with alias '%s'", channel->alias);
  FailOnInvalidNetChannel(channel);
  ParseURL(channel, record);
  g_hash_table_insert(netlist, GUINT_TO_POINTER(MakeKey(record)), record);
}

/* take channel connection info by channel alias */
static struct ChannelConnection *GetChannelConnectionInfo
  (const struct ChannelDesc *channel)
{
  struct ChannelConnection r; /* fake record */

  assert(channel != NULL);
  assert(netlist != NULL);

  ParseURL(channel, &r);
  return (struct ChannelConnection*)
      g_hash_table_lookup(netlist, GUINT_TO_POINTER(MakeKey(&r)));
}

/*
 * bind the given channel using info from netlist
 * returns not 0 if failed
 * note: replaces PrepareBind if no name service available
 */
static int DoBind(const struct ChannelDesc* channel)
{
  struct ChannelConnection *record;
  char buf[BIG_ENOUGH_SPACE], *url = buf;

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  MakeURL(url, BIG_ENOUGH_SPACE, channel, record);

  return zmq_bind(channel->socket, url);
}

/*
 * prepare "bind" channel information for the name service
 * note: should be called before name service invocation
 */
static void PrepareBind(struct ChannelDesc *channel)
{
  struct ChannelConnection *record;
  static uint16_t port = LOWEST_AVAILABLE_PORT;
  int result = 1;

  assert(channel != NULL);

  /* update netlist with the connection info */
  StoreChannelConnectionInfo(channel);

  /* if no name service is available just use given url and return */
  if(nameservice == NULL)
  {
    result = DoBind(channel);
    COND_ABORT(result != 0, "cannot bind the channel");
    return;
  }

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  /* in case channel has port assigned (full url) bind it and return */
  /* todo(d'b): perhaps reuse the code above? */
  if(record->port != 0)
  {
    result = DoBind(channel);
    COND_ABORT(result != 0, "cannot bind the channel");
    return;
  }

  /*
   * if channel does not have port pick up the port in the loop
   * todo(d'b): check upcoming zmq version for port 0 binding
   */
  assert(record != NULL);
  for(;port >= LOWEST_AVAILABLE_PORT; ++port)
  {
    record->port = port;
    result = DoBind(channel);
    if(result == OK_CODE) break;
  }

  COND_ABORT(result != OK_CODE, "cannot get the port to bind the channel");
  NaClLog(LOG_DEBUG, "%s; %s, %d: host = %u, port = %u",
      __FILE__, __func__, __LINE__, record->host, record->port);
}

/*
 * connect the given channel using info from netlist
 * returns not 0 if failed
 * note: replaces PrepareConnect if no name service available
 */
static int DoConnect(struct ChannelDesc* channel)
{
  struct ChannelConnection *record;
  char buf[BIG_ENOUGH_SPACE], *url = buf;

  record = GetChannelConnectionInfo(channel);
  assert(record != NULL);

  MakeURL(url, BIG_ENOUGH_SPACE, channel, record);
  NaClLog(LOG_DEBUG, "%s; %s, %d: url = %s", __FILE__, __func__, __LINE__, url);
  return zmq_connect(channel->socket, url);
}

/*
 * prepare "connect" channel information for the name service
 * note: will be called before name service invocation
 */
static void PrepareConnect(struct ChannelDesc* channel)
{
  int result;
  assert(channel != NULL);

  /* update netlist with the connection info */
  StoreChannelConnectionInfo(channel);

  /* if no name service is available just use given url and return */
  if(nameservice == NULL)
  {
    uint64_t hwm = 1; /* high water mark for PUSH socket to block on sending */

    result = zmq_setsockopt(channel->socket, ZMQ_HWM, &hwm, sizeof hwm);
    COND_ABORT(result != 0, "cannot set high water mark");

    result = DoConnect(channel);
    COND_ABORT(result != 0, "cannot bind the channel");
  }
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
 * return the real parcel size
 */
static int32_t ParcelCtor(const struct NaClApp *nap, char *parcel, const uint32_t size)
{
  char *p = parcel; /* parcel pointer */
  uint32_t node_id_network = bswap_32(nap->node_id); /* BIG ENDIAN */
  uint32_t binds_network = bswap_32(binds); /* BIG ENDIAN */
  uint32_t connects_network = bswap_32(connects); /* BIG ENDIAN */

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
static int32_t SendParcel(char *parcel, const uint32_t size)
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
  COND_ABORT(result == -1, "failed to send the parcel to the name server");

  /* receive the parcel back */
  result = recvfrom(ns_socket, parcel, size, 0, (void*)&ns, &ns_len);
  COND_ABORT(result == -1, "failed to receive the parcel from the name server");
  close(ns_socket);

  return result;
}

/* decode the parcel and update netlist */
static void DecodeParcel(const char *parcel, const uint32_t count)
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

/* poll the name server and update netlist with port information */
static void ResolveChannels(struct NaClApp *nap)
{
  char parcel[PARCEL_SIZE];
  int32_t parcel_size = PARCEL_SIZE;

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

/*
 * if name service is available then go through all available
 * channels, pick the network ones and connect them
 * note: if no name service is available quietly return - all channels
 * are already bound and connected due 1st pass
 */
void KickPrefetchChannels(struct NaClApp *nap)
{
  int i;

  /* quietly return if no name service specified */
  if(nameservice == NULL) return;

  assert(netlist != NULL);
  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* exchange channel information with the name server */
  ResolveChannels(nap);

  /* make connections */
  for(i = 0; i < nap->system_manifest->channels_count; ++i)
  {
    int result;
    struct ChannelDesc *channel = &nap->system_manifest->channels[i];
    struct ChannelConnection *record;

    assert(channel != NULL);

    /* skip all channels except the network "connect" ones */
    if(channel->source != NetworkChannel) continue;

    /* get the channel connection information */
    record = GetChannelConnectionInfo(channel);
    assert(record != NULL);

    /* only the "connect" channels need to be processed */
    if(record->mark != CONNECT_MARK) continue;

    /* bind or connect the channel and look at result */
    result = DoConnect(channel);
    COND_ABORT(result != 0, "cannot connect socket to address");
  }

  /*
   * temporary fix (the lost 1st messsage) to allow 0mq to complete
   * the connection procedure. 1 millisecond should be enough
   * todo(d'b): replace it with the channels readiness check
   */
  usleep(PREPOLL_WAIT);
}

/*
 * todo(d'b): move the code until this line to own file
 * end of the name service }}
 */

/*
 * check for an error. if encountered put it to log and
 * exit from the current function with standard error code
 * should be used in the nexe runtime instead of COND_ABORT()
 */
#define ZMQ_TEST_STATE(code, msg_ptr) \
    if((code) != 0)\
    {\
      NaClLog(LOG_ERROR, "zmq: error %d, %s\n",\
              zmq_errno(), zmq_strerror(zmq_errno()));\
      zmq_msg_close(msg_ptr);\
      return ERR_CODE;\
    }

/*
 * initiate networking (if there are network channels)
 * note: will run only once on the 1st channel construction
 */
static inline void NetCtor()
{
  struct ChannelDesc channel;

  /* context will be get at the very 1st call */
  if(channels_cnt++) return;

  /* get zmq context */
  context = zmq_init(1);
  COND_ABORT(context == NULL, "cannot initialize zeromq context");

  /*
   * initialize the name service table even if name service is not
   * specified because it is used to test channels engine errors
   * (still under construction) in the future if no name service
   * used the allocation can be removed
   */
  netlist = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, free);
  COND_ABORT(netlist == NULL, "cannot allocate netlist");

  /* get name service connection string if available */
  memset(&channel, 0, sizeof channel);
  channel.source = NetworkChannel;
  channel.name = GetValueByKey("NameServer");
  if(channel.name == NULL) return;

  /* parse the given string and make url (static var) */
  nameservice = malloc(sizeof *nameservice);
  COND_ABORT(nameservice == NULL, "cannot allocate memory to hold name server url");
  ParseURL(&channel, nameservice);
  COND_ABORT(nameservice->mark != OUTSIDER_MARK, "invalid name server type");
  COND_ABORT(nameservice->host == 0, "invalid name server host");
  COND_ABORT(nameservice->port == 0, "invalid name server port");
  COND_ABORT(nameservice->protocol != ChannelUDP, "only udp supported for name server");
}

/*
 * close all "push" channels manually after EOF's sent
 * note: temporary fix for zmq_term(). can be removed after zeromq
 *   team will fix it.
 * note: global nap object has been used. but it is ok since the patch
 *   is temporary
 */
static inline void CloseChannels()
{
  extern struct NaClApp *gnap;
  struct ChannelDesc *channels = gnap->system_manifest->channels;
  int busy;

  do
  {
    int i;
    busy = 0;

    for(i = 0; i < gnap->system_manifest->channels_count; ++i)
    {
      struct ChannelDesc *channel = &channels[i];
      if(channel->source == NetworkChannel && channel->limits[PutsLimit] != 0
          && channel->limits[PutSizeLimit] != 0)
      {
        uint32_t more;
        size_t more_size = sizeof more;

        if(channel->socket == NULL ) continue;

        zmq_getsockopt(channel->socket, ZMQ_EVENTS, &more, &more_size);
        busy |= more != ZMQ_POLLOUT;
        if(more == ZMQ_POLLOUT)
        {
          zmq_close(channel->socket);
          channel->socket = NULL;
        }
      }
    }
  } while(busy);
}

/*
 * finalize networking (if there are network channels)
 * note: will run only once. should be called from channel destructor
 */
static inline void NetDtor()
{
  /* context will be destroyed at the last call */
  if(--channels_cnt) return;

  /* temporary fix to make zmq_term() working */
  CloseChannels();

  /* terminate context */
  zmq_term(context);

  /* deallocate netlist */
  g_hash_table_destroy(netlist);

  /* deallocate name server record */
  free(nameservice);
}

/*
 * preallocate network channel. needs 2nd pass to complete
 * the channel allocation (connect)
 */
int PrefetchChannelCtor(struct ChannelDesc *channel)
{
  int sock_type;

  /* check for the design errors */
  assert(channel != NULL);
  assert(channel->source == NetworkChannel);

  /* log parameters and channel internals */
  NaClLog(LOG_DEBUG, "%s; %s, %d: alias = %s",
      __FILE__, __func__, __LINE__, channel->alias);

  /* explicitly reset network regarded fields */
  channel->socket = NULL;
  channel->bufend = 0;
  channel->bufpos = 0;

  /* open zmq socket. will run only 1st time */
  NetCtor();

  /* choose connection type and open socket */
  sock_type = channel->limits[GetsLimit]
      && channel->limits[GetSizeLimit] ? ZMQ_PULL : ZMQ_PUSH;
  channel->socket = zmq_socket(context, sock_type);
  COND_ABORT(channel->socket == NULL, "cannot obtain socket");

  /* bind or connect the channel */
  if(sock_type == ZMQ_PUSH)
  {
    PrepareConnect(channel);
    ++connects;
  }
  else
  {
    int result = zmq_msg_init(&channel->msg);
    ZMQ_TEST_STATE(result, &channel->msg);
    PrepareBind(channel);
    ++binds;
  }

  return OK_CODE;
}

/* check and update channel EOF state and etag */
static inline void UpdateChannelState(struct ChannelDesc *channel)
{
  int64_t more = 0;
  size_t more_size = sizeof more;

  zmq_getsockopt(channel->socket, ZMQ_RCVMORE, &more, &more_size);

  /* etag enabled */
  if(more != 0 && channel->bufend == TAG_DIGEST_SIZE - 1 && TagEngineEnabled())
  {
    /* store received digest */
    memcpy(channel->digest, zmq_msg_data(&channel->msg), TAG_DIGEST_SIZE);

    /* receive the zero part */
    zmq_recv(channel->socket, &channel->msg, 0);
    channel->bufend = zmq_msg_size(&channel->msg);
    if(channel->bufend == 0) channel->eof = 1;
    else NaClLog(LOG_ERROR, "invalid eof pattern detected");
  }

  /* etag disabled */
  if(more == 0 && channel->bufend == 0 && !TagEngineEnabled())
    channel->eof = 1;
}

/*
 * fetch the data from the network channel
 * return number of received bytes or error code
 */
int32_t FetchMessage(struct ChannelDesc *channel, char *buf, int32_t count)
{
  int32_t readrest = count;
  int32_t toread = 0;

  assert(channel != NULL);
  assert(buf != NULL);
  assert(channel->bufpos <= NET_BUFFER_SIZE);
  assert(channel->bufend >= 0);
  assert(channel->bufend <= NET_BUFFER_SIZE);

  /*
   * read message part by part until "count" not reached.
   * the part size defined as 64kb
   */
  for(readrest = count; readrest > 0; readrest -= toread)
  {
    /* calculates available data, if needed update buffer and restart loop */
    toread = MIN(readrest, channel->bufend - channel->bufpos);
    if(channel->eof && toread == 0) break;

    if(toread == 0)
    {
      int result;

      /* re-initialize message and rewind the channel buffer */
      zmq_msg_close(&channel->msg);
      channel->bufpos = 0;
      result = zmq_msg_init(&channel->msg);
      ZMQ_TEST_STATE(result, &channel->msg);

      /* read the next message */
      result = zmq_recv(channel->socket, &channel->msg, 0);
      ZMQ_TEST_STATE(result, &channel->msg);
      channel->bufend = zmq_msg_size(&channel->msg);

      UpdateChannelState(channel);
      continue;
    }

    /* there is the data to take */
    memcpy(buf, (char*)zmq_msg_data(&channel->msg) + channel->bufpos, toread);
    channel->bufpos += toread;
    buf += toread;
  }

  return count - readrest;
}

/*
 * send the data to the network channel
 * return number of sent bytes or error code
 */
int32_t SendMessage(struct ChannelDesc *channel, const char *buf, int32_t count)
{
  int result;
  int32_t writerest;
  int32_t flag;
  char url[BIG_ENOUGH_SPACE];  /* debug purposes only */

  assert(channel != NULL);
  assert(buf != NULL);

  /*
   * log parameters and channel internals
   * todo(d'b): remove MakeURL. at least for verbosity < then LOG_DEBUG
   */
  MakeURL(url, BIG_ENOUGH_SPACE, channel, GetChannelConnectionInfo(channel));
  NaClLog(LOG_DEBUG, "%s; %s, %d: alias = %s, url = %s",
      __FILE__, __func__, __LINE__, channel->alias, url);

  /* write EOF as a multi-part message if etag enabled */
  flag = channel->eof ? ZMQ_SNDMORE : 0;

  /* send a buffer through the multiple messages */
  for(writerest = count; writerest > 0; writerest -= NET_BUFFER_SIZE)
  {
    int32_t towrite = MIN(writerest, NET_BUFFER_SIZE);

    /*
     * since 0mq is not able to send the messages synchronously,
     * "zero copy" cannot be used
     * todo(d'b): limit the queue to 1 message, send given data in chunks
     *   as "zero copy" and copy the remaining chunk to buffer allocated
     *   by zerovm so it can be sent asynchronously
     */

    /* create the message */
    result = zmq_msg_init_size(&channel->msg, towrite);
    ZMQ_TEST_STATE(result, &channel->msg);
    memcpy(zmq_msg_data(&channel->msg), buf, towrite);

    /* send the message */
    result = zmq_send(channel->socket, &channel->msg, flag);
    ZMQ_TEST_STATE(result, &channel->msg);

    /* destroy the message (caring about EOF) */
    buf += towrite;
    if(channel->eof == 0) zmq_msg_close(&channel->msg);
  }

  /* if sending EOF */
  if(channel->eof)
  {
    /* create the message if didn't */
    if(count == 0)
    {
      result = zmq_msg_init(&channel->msg);
      ZMQ_TEST_STATE(result, &channel->msg);
    }

    result = zmq_msg_init_size(&channel->msg, 0);
    ZMQ_TEST_STATE(result, &channel->msg);
    result = zmq_send(channel->socket, &channel->msg, 0);
    ZMQ_TEST_STATE(result, &channel->msg);
    zmq_msg_close(&channel->msg);
  }

  return count;
}

/*
 * finalize and deallocate network channel
 * todo(d'b): rewrite the code after zmq_term will be fixed
 */
int PrefetchChannelDtor(struct ChannelDesc *channel)
{
  char url[BIG_ENOUGH_SPACE];  /* debug purposes only */

  assert(channel != NULL);
  assert(channel->socket != NULL);

  /* log parameters and channel internals */
  MakeURL(url, BIG_ENOUGH_SPACE, channel, GetChannelConnectionInfo(channel));
  NaClLog(LOG_DEBUG, "%s; %s, %d: alias = %s, url = %s",
      __FILE__, __func__, __LINE__, channel->alias, url);

  /* close "PUT" channel */
  if(channel->limits[PutsLimit] && channel->limits[PutSizeLimit])
  {
    int size = 0;

    /* eof must be set before tag sending */
    channel->eof = 1;

    /* send channel etag digest (if enabled) */
    if(TagEngineEnabled())
    {
      size = TAG_DIGEST_SIZE - 1; /* don't send ending '\0' */
      TagDigest(&channel->tag, channel->digest);
    }
    SendMessage(channel, channel->digest, size);

  }

  /* close "GET" channel */
  if(channel->limits[GetsLimit] && channel->limits[GetSizeLimit])
  {
    /* wind the channel to the end */
    while(channel->eof == 0)
    {
      char buf[NET_BUFFER_SIZE];
      FetchMessage(channel, buf, NET_BUFFER_SIZE);
    }

    /* test integrity (if etag enabled) */
    if(TagEngineEnabled())
    {
      char local_digest[TAG_DIGEST_SIZE];

      TagDigest(channel->tag, local_digest);
      if(memcmp(local_digest, channel->digest, TAG_DIGEST_SIZE - 1) == 0)
        NaClLog(LOG_DEBUG, "channel %s is ok. tag = %s", channel->alias, local_digest);
      else
        NaClLog(LOG_ERROR, "channel %s is corrupted. tags = %s : %s",
            channel->alias, local_digest, channel->digest);
    }

    zmq_msg_close(&channel->msg);
    zmq_close(channel->socket);
  }

  /* will destroy context and netlist after all network channels closed */
  NetDtor();

  return OK_CODE;
}
