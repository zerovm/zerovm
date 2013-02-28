/*
 * routines to calculate hashes. should be constructed from the command
 * line parser. doesn't need destructor.
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "src/main/tools.h"
#include "src/main/etag.h"

static int tag_engine_enabled = 0;

/* etag engine construction */
void TagEngineCtor()
{
  tag_engine_enabled = 1;
}

/* disable etag */
void TagEngineDtor()
{
  tag_engine_enabled = 0;
}

/* return the etag_enabled state */
int TagEngineEnabled()
{
  return tag_engine_enabled;
}

/*
 * initialize and return the hash context or abort if failed
 * to avoid memory leak context must be freed after usage
 */
void *TagCtor()
{
  GChecksum *ctx;

  ctx = g_checksum_new(TAG_ENCRYPTION);
  ZLOGFAIL(ctx == NULL, EFAULT, "error initializing tag context");
  return ctx;
}

/* deallocate tag context */
void TagDtor(void *ctx)
{
  g_checksum_free(ctx);
}

/*
 * calculates digest from the context. can be used consequently
 * note: "digest" must have enough space to hold the digest
 */
void TagDigest(void *ctx, char *digest)
{
  const char *hex;
  GChecksum *tmp;

  tmp = g_checksum_copy(ctx);
  hex = g_checksum_get_string(tmp);
  strcpy(digest, hex);
  g_checksum_free(tmp);
}

/*
 * update tag with the given buffer.
 * returns 0 if all ok or -1 if failed
 */
void TagUpdate(void *ctx, const char *buffer, int32_t size)
{
  assert(ctx != NULL);
  assert(buffer != NULL);
  assert(size >= 0);

  /* update the context with a new data */
  if(size > 0)
    g_checksum_update(ctx, (const guchar*)buffer, size);
}
