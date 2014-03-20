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

#define MFTFAIL ZLogTag("MANIFEST", cline), FailIf

/* general */
#define PTR_SIZE (sizeof(void*))
#define MANIFEST_VERSION "20140320"
#define MANIFEST_SIZE_LIMIT 0x80000
#define MANIFEST_LINES_LIMIT 0x2000
#define MANIFEST_TOKENS_LIMIT 0x10
#define NAME_PREFIX_NET "net:"

/* delimiters */
#define LINE_DELIMITER "\n"
#define KEY_DELIMITER "="
#define VALUE_DELIMITER ","
#define TOKEN_DELIMITER ";"
#define CONNECTION_DELIMITER ":"

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
  X(Node, 0, 1) \
  X(Job, 0, 1) \
  X(Broker, 0, 1)

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
static int cline = 0;

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
  MFTFAIL(*a != '\0' || errno != 0, EFAULT,
      "invalid numeric value '%s'", a);

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
  MFTFAIL(g_strcmp0(MANIFEST_VERSION, g_strstrip(value)) != 0,
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
  MFTFAIL(tokens[MemoryTokensNumber] != NULL || tokens[MemoryTag] == NULL,
      EFAULT, "invalid memory token");

  manifest->mem_size = ToInt(tokens[MemorySize]);
  tag = ToInt(tokens[MemoryTag]);

  /* initialize manifest field */
  MFTFAIL(tag != 0 && tag != 1, EFAULT, "invalid memory etag token");

  manifest->mem_tag = tag == 0 ? NULL : TagCtor();
  g_strfreev(tokens);
}

static void Timeout(struct Manifest *manifest, char *value)
{
  manifest->timeout = ToInt(value);
}

static void Node(struct Manifest *manifest, char *value)
{
  manifest->node = g_strdup(g_strstrip(value));
}

static void Broker(struct Manifest *manifest, char *value)
{
  manifest->broker = g_strdup(g_strstrip(value));
}

static void Job(struct Manifest *manifest, char *value)
{
  MFTFAIL(strlen(value) > UNIX_PATH_MAX, EFAULT, "too long Job path");
  manifest->job = g_strdup(g_strstrip(value));
}

/* set channels field */
static void Channel(struct Manifest *manifest, char *value)
{
  int i;
  char **tokens;
  struct ChannelDesc *channel = g_malloc0(sizeof *channel);

  /* get tokens from channel description */
  tokens = g_strsplit(value, VALUE_DELIMITER, ChannelTokensNumber + 1);
  for(i = 0; tokens[i] != NULL; ++i)
    g_strstrip(tokens[i]);

  MFTFAIL(tokens[ChannelTokensNumber] != NULL || tokens[PutSize] == NULL,
      EFAULT, "invalid channel tokens number");

  /*
   * set name and protocol (network or local). exact protocol
   * for the local channel will be set later
   */
  if(g_str_has_prefix(tokens[Name], NAME_PREFIX_NET))
  {
    channel->protocol = ProtoSocket;
    channel->name = g_strdup(tokens[Name] + strlen(NAME_PREFIX_NET));
  }
  else
  {
    MFTFAIL(!g_path_is_absolute(tokens[Name]), EFAULT,
        "only absolute path channels are allowed");
    channel->protocol = ProtoRegular;
    channel->name = g_strdup(tokens[Name]);
  }

  /* initialize the rest of fields */
  channel->alias = g_strdup(tokens[Alias]);
  channel->handle = NULL;
  channel->type = ToInt(tokens[Type]);

  /* parse limits */
  for(i = 0; i < LimitsNumber; ++i)
  {
    channel->limits[i] = ToInt(tokens[i + Gets]);
    MFTFAIL(channel->limits[i] < 0, EFAULT,
        "negative limits for %s", channel->alias);
  }

  /* append a new channel */
  g_ptr_array_add(manifest->channels, channel);
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
    cline = i + 1;

    tokens = g_strsplit(lines[i], KEY_DELIMITER, MANIFEST_TOKENS_LIMIT);
    if(strlen(lines[i]) > 0 && tokens[Key] != NULL
        && tokens[Value] != NULL && tokens[KeyValueTokensNumber] == NULL)
    {
      /* switch invoking functions by the keyword */
#define XSWITCH(a) switch(GetKey(tokens[Key])) {a};
#define X(a, o, s) case Key##a: ++counters[Key##a]; a(manifest, tokens[Value]); \
    MFTFAIL(*tokens[Value] == 0, EFAULT, "%s has empty value", tokens[Key]); break;
      XSWITCH(KEYWORDS)
#undef X
    }
    g_strfreev(tokens);
  }

  /* check obligatory and singleton keywords */
  g_strfreev(lines);
  CheckCounters(counters, XSIZE(KEYWORDS));

  /* "Broker" depends on "Node" existence */
  ZLOGFAIL(manifest->broker != NULL && manifest->node == NULL,
      EFAULT, "broker specified but node is absent");

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

  if(manifest == NULL) return;

  /* channels */
  for(i = 0; i < manifest->channels->len; ++i)
  {
    struct ChannelDesc *channel = g_ptr_array_index(manifest->channels, i);

    g_free(channel->alias);
    g_free(channel->name);
    g_free(channel);
  }
  g_ptr_array_free(manifest->channels, TRUE);

  /* other */
  g_free(manifest->broker);
  TagDtor(manifest->mem_tag);
  g_free(manifest->program);
  g_free(manifest);
}
