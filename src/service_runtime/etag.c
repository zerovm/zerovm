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
#include "src/service_runtime/tools.h"
#include "src/service_runtime/etag.h"
#include "api/zvm.h" /* ERR_CODE and OK_CODE */

static int tag_engine_enabled = 0;
static const EVP_MD *tag_type = NULL;

/* etag engine construction */
void TagEngineCtor()
{
  tag_engine_enabled = 1;
  OpenSSL_add_all_digests();
  tag_type = EVP_get_digestbyname(TAG_ENCRYPTION);
  COND_ABORT(tag_type == NULL, "cannot initialize tag type");
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
  assert(tag_type != NULL);

  EVP_DigestInit(ctx, tag_type);
  ErrIf(ctx == NULL, "error initializing hash context");
  return OK_CODE;
}

/*
 * calculates digest from the context. can be used consequently
 * note: "digest" must have enough space to hold the digest
 */
void TagDigest(void *ctx, char *digest)
{
  unsigned char p[TAG_BINARY_SIZE] = {0};
  EVP_MD_CTX tag;
  unsigned dsize;
  int i;

  assert(ctx != NULL);
  assert(digest != NULL);

  /* copy context aside and finalize it to get digest */
  i = EVP_MD_CTX_copy(&tag, ctx);
  ErrIf(i != 1, "error copying tag");
  EVP_DigestFinal(&tag, p, &dsize);
  for(i = 0; i < dsize; ++i)
    sprintf(&digest[2*i], "%02X", p[i]);
}

/*
 * update etag with the given buffer.
 * returns 0 if all ok or -1 if failed
 */
int TagUpdate(void *ctx, const char *buffer, int32_t size)
{
  assert(ctx != NULL);
  assert(buffer != NULL);
  assert(size >= 0);
  if(size == 0) return OK_CODE;

  /* update the context with a new data */
  EVP_DigestUpdate(ctx, buffer, size);
  return OK_CODE;
}
