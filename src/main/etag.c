/*
 * routines to calculate hashes. should be constructed from the command
 * line parser. doesn't need destructor.
 *
 *  Created on: Sep 15, 2012
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "src/main/tools.h"
#include "src/main/etag.h"
#include "api/zvm.h" /* ERR_CODE and OK_CODE */

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
