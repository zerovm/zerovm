/*
 * the manifest file format: key = value. parser ignores white spaces
 * each line can only contain single key=value
 *
 * the api is very simple: just call GetValuByKey() after
 * manifest is constructed
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

/*
 * manifest parser. input: manifest file name. output: manifest structure
 */
#include <assert.h>
#include <arpa/inet.h> /* convert ip to int */
#include "src/main/manifest.h"
#include "src/channels/channel.h"

/* general */
#define PTR_SIZE (sizeof(void*))
#define MANIFEST_VERSION "20130611"
#define MANIFEST_SIZE_LIMIT 0x80000
#define MANIFEST_LINES_LIMIT 0x2000
#define MANIFEST_TOKENS_LIMIT 0x10

/* delimiters */
#define LINE_DELIMITER "\n"
#define KEY_DELIMITER "="
#define VALUE_DELIMITER ","
#define TOKEN_DELIMITER ";"
#define CONNECTION_DELIMITER ":"

#define XARRAY(a) static char *ARRAY_##a[] = {a};
#define X(a) #a,
  XARRAY(PROTOCOLS)
#undef X

/* key/value tokens */
typedef enum {
  Key,
  Value,
  KeyValueTokensNumber
} KeyValueTokens;

/* memory tokens */
typedef enum {
  MemorySize,
  MemoryTag,
  MemoryTokensNumber
} MemoryTokens;

/* connection tokens */
typedef enum {
  Protocol,
  Host,
  Port,
  ConnectionTokensNumber
} ConnectionTokens;

/* channel tokens */
typedef enum {
  Name,
  Alias,
  Type,
  Tag,
  Gets,
  GetSize,
  Puts,
  PutSize,
  ChannelTokensNumber
} ChannelTokens;

/* (x-macro): manifest keywords (name, obligatory, singleton) */
#define KEYWORDS \
  X(Channel, 1, 0) \
  X(Version, 1, 1) \
  X(Program, 1, 1) \
  X(Memory, 1, 1) \
  X(Timeout, 1, 1) \
  X(NameServer, 0, 1) \
  X(Node, 0, 1) \
  X(Job, 0, 1) \
  X(Etag, 0, 1)

/* (x-macro): manifest enumeration, array and statistics */
#define XENUM(a) enum ENUM_##a {a};
#define X(a, o, s) Key ## a,
  XENUM(KEYWORDS)
#undef X

#define XARRAY(a) static char *ARRAY_##a[] = {a};
#define X(a, o, s) #a,
  XARRAY(KEYWORDS)
#undef X

#define XCHECK(a) static uint8_t CHECK_##a[] = {a};
#define X(a, o, s) o | (s << 1),
  XCHECK(KEYWORDS)
#undef X

/*
 * LOCAL variables and functions
 */
/* get manifest data */
static void GetManifestData(const char *name, char *buf)
{
  FILE *h = fopen(name, "r");

  ZLOGFAIL(h == NULL, ENOENT, "manifest open error");
  ZLOGFAIL(fread(buf, 1, MANIFEST_SIZE_LIMIT, h) < 1, EIO, "manifest read error");
  fclose(h);
}

int64_t ToInt(char *a)
{
  int64_t result;

  errno = 0;
  a = g_strstrip(a);
  result = g_ascii_strtoll(a, &a, 0);
  ZLOGFAIL(*a != '\0' || errno != 0, EFAULT, "invalid numeric value '%s'", a);

  return result;
}

/*
 * return keyword and update given "value" with extracted value
 * spaces will be trimmed from "value"
 */
static XTYPE(KEYWORDS) GetKey(char *key)
{
  XTYPE(KEYWORDS) k;

  key = g_strstrip(key);
  for(k = 0; k < XSIZE(KEYWORDS); ++k)
    if(g_strcmp0(XSTR(KEYWORDS, k), key) == 0) return k;
  return -1;
}

/* test manifest version */
static void Version(struct Manifest *manifest, char *value)
{
  ZLOGFAIL(g_strcmp0(MANIFEST_VERSION, g_strstrip(value)) != 0,
      EFAULT, "invalid manifest version");
}

/* set program field (should be g_free later) */
static void Program(struct Manifest *manifest, char *value)
{
  manifest->program = g_strdup(g_strstrip(value));
}

/* set mem_size and mem_tag field */
static void Memory(struct Manifest *manifest, char *value)
{
  char **tokens;
  int tag;

  /* parse value */
  tokens = g_strsplit(value, VALUE_DELIMITER, MemoryTokensNumber);
  ZLOGFAIL(tokens[MemoryTokensNumber] != NULL || tokens[MemoryTag] == NULL,
      EFAULT, "invalid memory token");

  manifest->mem_size = ToInt(tokens[MemorySize]);
  tag = ToInt(tokens[MemoryTag]);

  /* initialize manifest field */
  ZLOGFAIL(tag != 0 && tag != 1, EFAULT, "invalid memory etag token");

  manifest->mem_tag = tag == 0 ? NULL : TagCtor();
  g_strfreev(tokens);
}

static void Timeout(struct Manifest *manifest, char *value)
{
  manifest->timeout = ToInt(value);
}

static void Node(struct Manifest *manifest, char *value)
{
  manifest->node = ToInt(value);
}

static void Job(struct Manifest *manifest, char *value)
{
  ZLOGFAIL(strlen(value) > UNIX_PATH_MAX, EFAULT, "too long Job name");
  manifest->job = g_strdup(g_strstrip(value));
}

static void Etag(struct Manifest *manifest, char *value)
{
  manifest->etag = g_strdup(g_strstrip(value));
}

/* convert ip address (or node id) to integer */
static uint32_t ExtractHost(char *host, uint8_t *flags)
{
  uint32_t result;

  *flags = 0;
  if(strchr(host, '.') != NULL)
  {
    struct sockaddr_in sa;
    *flags = 1;
    result = inet_pton(AF_INET, g_strstrip(host), &sa.sin_addr);
    result = result == 1 ? sa.sin_addr.s_addr : 0;
    ZLOGFAIL(result == 0, EFAULT, "malformed ip token");
  }
  else
    result = ToInt(host);

  return result;
}

/* analyze given string and return protocol id */
static XTYPE(PROTOCOLS) GetChannelProtocol(char *proto)
{
  XTYPE(PROTOCOLS) p;

  proto = g_strstrip(proto);
  for(p = 0; p < XSIZE(PROTOCOLS); ++p)
    if(g_ascii_strcasecmp(XSTR(PROTOCOLS, p), proto) == 0) return p;
  return -1;
}

/* parse the name and append to given array of names as connection or string */
static void ParseName(char *name, GPtrArray *names)
{
  char **tokens;
  XTYPE(PROTOCOLS) proto;

  name = g_strstrip(name);
  tokens = g_strsplit(name, CONNECTION_DELIMITER, ConnectionTokensNumber);
  proto = GetChannelProtocol(tokens[Protocol]);

  if(proto == -1)
  {
    struct File *f;
    ZLOGFAIL(tokens[1] != NULL, EFAULT, "invalid channel name");
    ZLOGFAIL(!g_path_is_absolute(name), EFAULT,
        "only absolute path channels are allowed");


    f = g_malloc0(sizeof *f);
    f->protocol = ProtoRegular; /* just in case (will be set later) */
    f->name = g_strdup(name);
    f->handle = NULL;
    g_ptr_array_add(names, f);
  }
  else
  {
    struct Connection *c = g_malloc0(sizeof *c);

    /* TODO(d'b): fix "invalid_name_server_type.manifest" bug here */
    ZLOGFAIL(tokens[ConnectionTokensNumber] != NULL || tokens[Host] == NULL,
        EFAULT, "invalid channel url");

    c->protocol = proto;
    c->host = ExtractHost(tokens[Host], &c->flags);
    c->port = ToInt(tokens[Port]);
    c->handle = NULL;
    g_ptr_array_add(names, c);
  }

  g_strfreev(tokens);
}

/* TODO(d'b): it is ugly. solution needed */
static void NameServer(struct Manifest *manifest, char *value)
{
  GPtrArray *dummy = g_ptr_array_new();
  ParseName(value, dummy);
  manifest->name_server = g_ptr_array_index(dummy, 0);
  g_ptr_array_free(dummy, TRUE);
}

/* set channels field */
static void Channel(struct Manifest *manifest, char *value)
{
  char **tokens;
  char **names;
  int i;
  struct ChannelDesc *channel;

  /* allocate a new channel */
  channel = g_malloc0(sizeof *channel);
  channel->source = g_ptr_array_new();

  /* get tokens from channel description */
  tokens = g_strsplit(value, VALUE_DELIMITER, ChannelTokensNumber);

  /* TODO(d'b): fix "invalid numeric value ', 0'" bug here */
  ZLOGFAIL(tokens[ChannelTokensNumber] != NULL || tokens[PutSize] == NULL,
      EFAULT, "invalid channel tokens number");

  /* parse alias and name(s) */
  channel->alias = g_strdup(g_strstrip(tokens[Alias]));
  names = g_strsplit(tokens[Name], TOKEN_DELIMITER, MANIFEST_TOKENS_LIMIT);
  for(i = 0; names[i] != NULL; ++i)
    ParseName(names[i], channel->source);

  channel->type = ToInt(tokens[Type]);

  i = ToInt(tokens[Tag]);
  ZLOGFAIL(i != 0 && i != 1, EFAULT, "invalid channel mumeric token");

  channel->tag = i == 0 ? NULL : TagCtor();

  for(i = 0; i < LimitsNumber; ++i)
  {
    channel->limits[i] = ToInt(tokens[i + Gets]);
    ZLOGFAIL(channel->limits[i] < 0, EFAULT,
        "negative limits for %s", channel->alias);
  }

  /* append a new channel */
  g_ptr_array_add(manifest->channels, channel);
  g_strfreev(names);
  g_strfreev(tokens);
}

/*
 * check if obligatory keywords appeared and check if the fields
 * which should appear only once did so
 */
static void CheckCounters(int *counters, int n)
{
  while(--n)
  {
    ZLOGFAIL(counters[n] == 0 && (CHECK_KEYWORDS[n] & 1),
        EFAULT, "%s not specified", XSTR(KEYWORDS, n));
    ZLOGFAIL(counters[n] > 1 && (CHECK_KEYWORDS[n] & 2),
        EFAULT, "duplicate %s keyword", XSTR(KEYWORDS, n));
  }
}

struct Manifest *ManifestTextCtor(char *text)
{
  struct Manifest *manifest = g_malloc0(sizeof *manifest);
  int counters[XSIZE(KEYWORDS)] = {0};
  char **lines;
  int i;

  /* initialize channels */
  manifest->channels = g_ptr_array_new();

  /* extract all lines */
  lines = g_strsplit(text, LINE_DELIMITER, MANIFEST_LINES_LIMIT);

  /* parse each line */
  for(i = 0; lines[i] != NULL; ++i)
  {
    char **tokens;

    tokens = g_strsplit(lines[i], KEY_DELIMITER, MANIFEST_TOKENS_LIMIT);
    if(strlen(lines[i]) > 0 && tokens[Key] != NULL
        && tokens[Value] != NULL && tokens[KeyValueTokensNumber] == NULL)
    {
      /* switch invoking functions by the keyword */
#define XSWITCH(a) switch(GetKey(tokens[Key])) {a};
#define X(a, o, s) case Key##a: ++counters[Key##a]; a(manifest, tokens[Value]); break;
      XSWITCH(KEYWORDS)
#undef X
    }
    g_strfreev(tokens);
  }

  /* check obligatory and singleton keywords */
  g_strfreev(lines);
  CheckCounters(counters, XSIZE(KEYWORDS));

  return manifest;
}

struct Manifest *ManifestCtor(const char *name)
{
  char buf[MANIFEST_SIZE_LIMIT] = {0};
  GetManifestData(name, buf);
  return ManifestTextCtor(buf);
}

void ManifestDtor(struct Manifest *manifest)
{
  int i;
  int j;

  if(manifest == NULL) return;

  /* channels */
  for(i = 0; i < manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = g_ptr_array_index(manifest->channels, i);

    /* sources */
    for(j = 0; j < channel->source->len; ++j)
    {
      struct File *file = g_ptr_array_index(channel->source, j);
      if(IS_FILE(file))
        g_free(file->name);
      g_free(file);
    }
    g_ptr_array_free(channel->source, TRUE);
    g_free(channel->alias);
    TagDtor(channel->tag);
    g_free(channel);
  }
  g_ptr_array_free(manifest->channels, TRUE);

  /* other */
  g_free(manifest->etag);
  TagDtor(manifest->mem_tag);
  g_free(manifest->name_server);
  g_free(manifest->program);
  g_free(manifest);
}
