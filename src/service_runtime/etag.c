/*
 * routines to calculate hashes. should be constructed from the command
 * line parser. doesn't need destructor.
 * API:
 *  void EtagCtor(struct NaClApp *nap)
 *  const char *EtagToText(unsigned char *p)
 *  const unsigned char *OverallEtag(SHA_CTX *ctx)
 *  const char *UpdateEtag(SHA_CTX *etag, char *buffer, int32_t size)
 *
 *  Created on: Sep 15, 2012
 *      Author: d'b
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "src/service_runtime/tools.h"
#include "src/service_runtime/etag.h"
#include "api/zvm.h" /* ERR_CODE and OK_CODE */

static int tag_engine_enabled = 0;

/* etag engine construction */
void TagEngineCtor()
{
  tag_engine_enabled = 1;
}

/* return the etag_enabled state */
int TagEngineEnabled()
{
  return tag_engine_enabled;
}

/*
 * initialize the hash context in provided space
 * return 0 if everything is ok
 */
int TagCtor(void *ctx)
{
  int code;

  code = SHA1_Init((SHA_CTX*)ctx);
  ErrIf(code == 0, "error initializing hash context");
  return code == 0 ? ERR_CODE : OK_CODE;
}

/*
 * calculates digest from the context. can be used consequently
 * note: "digest" must have enough space to hold the digest
 */
void TagDigest(void *ctx, char *digest)
{
  unsigned char p[TAG_BINARY_SIZE] = {'B', 'A', 'D', 0};
  SHA_CTX tag;
  int code;

  assert(ctx != NULL);
  assert(digest != NULL);

  /* copy context aside and finalize it to get digest */
  memcpy(&tag, ctx, TAG_CONTEXT_SIZE);
  code = SHA1_Final(p, &tag);
  ErrIf(code != 1, "error finalizing etag");

  sprintf(digest, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
      "%02X%02X%02X%02X%02X", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
      p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19]);
}

/*
 * update etag with the given buffer.
 * returns 0 if all ok or -1 if failed
 */
int TagUpdate(void *ctx, const char *buffer, int32_t size)
{
  int code;

  assert(ctx != NULL);
  assert(buffer != NULL);
  assert(size > 0);

  /* update the context with a new data */
  code = SHA1_Update((SHA_CTX*)ctx, buffer, size);
  ErrIf(code != 1, "error updating etag");
  return code != 1 ? ERR_CODE : OK_CODE;
}
