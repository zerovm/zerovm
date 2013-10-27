/*
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
#include <arpa/inet.h> /* ip <-> int */
#include <assert.h>
#include <errno.h>
#include "src/channels/channel.h"
#include "src/channels/nservice.h"

/*
 * name server expects all data in the big endian. but on the big
 * endian platform name service data doesn't need to be converted
 */
#if BYTE_ORDER == BIG_ENDIAN
#define bswap_16(a)
#define bswap_32(a)
#define bswap_64(a)
#else
#include <byteswap.h>
#endif

#define RETRY 3 /* poll retries */
#define TIMEOUT 1200000 /* poll timeout in microseconds */
#define PARCEL_SIZE 65507 /* maximum size of an UDP packet */

/*
 * parcel will have at least 1 record or will not send
 * TODO(d'b): to prevent the structure padding the pragma was used
 * develop some more portable way (casting to array is ugly)
 */

/* host is node id when sending to NS and ip when receiving */
#pragma pack(push, 1)
struct NSRecord
{
  uint32_t host;
  uint16_t port;
};

struct NSParcel
{
  uint32_t node;
  uint32_t bind_number;
  uint32_t connect_number;
  struct NSRecord records[1];
};
#pragma pack(pop)

/* get next bind/connect source */
#define NEXT_SRC() \
    do { \
      c = CH_CONN(CH_CH(manifest, m), n++); \
      if(n == CH_CH(manifest, m)->source->len) \
      { \
        ++m; \
        n = 0; \
        continue; \
      } \
    } while(c->protocol != ProtoTCP && IS_IPHOST(c))

/* serialize channels data to the parcel. return parcel and its "size" */
static void *ParcelCtor(const struct Manifest *manifest,
    uint32_t *size, uint32_t binds, uint32_t connects)
{
  struct NSParcel *p;
  int64_t end = binds + connects;
  int64_t m = 0, n = 0;
  int64_t i;

  /* allocate parcel */
  *size = sizeof *p + (end - 1) * sizeof(struct NSRecord);
  p = g_malloc(*size);

  /* parcel header */
  p->node = bswap_32(manifest->node);
  p->bind_number = bswap_32(binds);
  p->connect_number = bswap_32(connects);

  /* populate parcel with bind/connect records */
  for(i = 0; i < end; ++i)
  {
    struct Connection *c;
    NEXT_SRC();
    p->records[i].host = bswap_32(c->host);
    p->records[i].port = bswap_16(c->port);
  }

  return (void*)p;
}

/* de-serialize channels data from the parcel. return number of sources */
static int ParcelDtor(struct Manifest *manifest, char *parcel)
{
  struct NSParcel *p = (void*)parcel;
  int64_t end = bswap_32(p->bind_number) + bswap_32(p->connect_number);
  int64_t m = 0, n = 0;
  int64_t i;

  /* update channels sources from the parcel */
  for(i = 0; i < end; ++i)
  {
    struct Connection *c;
    NEXT_SRC();

    /* skip "binds" */
    if(i < bswap_32(p->bind_number)) continue;

    /* ip is already in network format for inet_ntoa */
    c->host = p->records[i].host;
    c->port = bswap_16(p->records[i].port);
  }

  g_free(parcel);
  return end;
}
#undef NEXT_SRC

/*
 * send the parcel to the name server and get it back
 * return the received parcel size
 */
static int32_t PollServer(const struct Connection *server, char *parcel, uint32_t size)
{
  int i;
  int s;
  int32_t result;
  struct sockaddr_in addr;
  uint32_t len = sizeof addr;
  struct timeval timeout = {TIMEOUT / MICRO_PER_SEC, TIMEOUT % MICRO_PER_SEC};

  /* connect to the name server */
  s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  addr.sin_addr.s_addr = server->host;
  addr.sin_port = bswap_16(server->port);
  addr.sin_family = AF_INET;

  /* set timeout on socket i/o */
  result = setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof timeout);
  ZLOGFAIL(result == -1, errno, "cannot set i/o timeout");

  /* poll server */
  for(i = 0; i < RETRY; ++i)
  {
    result = sendto(s, parcel, size, 0, &addr, sizeof addr);
    if(result < 0) continue;
    result = recvfrom(s, parcel, size, 0, &addr, &len);
    if(result > 0) break;
  }

  ZLOGIF(i && result > 0, "name service polled with %d retries", i);
  ZLOGFAIL(result == -1, errno, "name service failed");
  close(s);
  return result;
}

void NameServiceCtor(struct Manifest *manifest, uint32_t b, uint32_t c)
{
  char *parcel;
  uint32_t psize;
  uint32_t control_psize;
  uint32_t control_nsources;

  assert(manifest != NULL);
  assert(manifest->channels != NULL);
  assert(b + c <= MAX_CHANNELS_NUMBER);

  /* return if there is no name service or network sources */
  if(manifest->name_server == NULL) return;
  if(b + c < 1) return;

  /* create parcel */
  ZLOGFAIL(manifest->name_server->protocol != ProtoUDP,
      EFAULT, "name server only support udp protocol");
  parcel = ParcelCtor(manifest, &psize, b, c);

  /* exchange parcels with name server */
  control_psize = PollServer(manifest->name_server, parcel, psize);
  ZLOGFAIL(control_psize != psize, EFAULT,
      "received parcel size %u is not equal to sent one %u",
      control_psize, psize);

  /* decode received parcel to channels */
  control_nsources = ParcelDtor(manifest, parcel);
  ZLOGFAIL(control_nsources != b + c, EFAULT,
      "received parcel records %u is not equal to sent ones %u",
      control_nsources, b + c);
}

void NameServiceDtor()
{
}
