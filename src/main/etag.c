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

void *TagCtor()
{
  GChecksum *ctx;

  ctx = g_checksum_new(TAG_ENCRYPTION);
  ZLOGFAIL(ctx == NULL, EFAULT, "error initializing tag context");
  return ctx;
}

void TagDtor(void *ctx)
{
  g_checksum_free(ctx);
}

void TagDigest(void *ctx, char *digest)
{
  const char *hex;
  GChecksum *tmp;

  tmp = g_checksum_copy(ctx);
  hex = g_checksum_get_string(tmp);
  strcpy(digest, hex);
  g_checksum_free(tmp);
}

void TagUpdate(void *ctx, const char *buffer, int64_t size)
{
  assert(ctx != NULL);
  assert(buffer != NULL);
  assert(size >= 0);

  /* update the context with a new data */
  if(size > 0)
    g_checksum_update(ctx, (const guchar*)buffer, size);
}
