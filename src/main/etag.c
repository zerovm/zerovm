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
static const EVP_MD *tag_type = NULL;

/* etag engine construction */
void TagEngineCtor()
{
  tag_engine_enabled = 1;
  OpenSSL_add_all_digests();
  tag_type = EVP_get_digestbyname(TAG_ENCRYPTION);
  ZLOGFAIL(tag_type == NULL, EFAULT, "cannot initialize tag type");
}

/* disable etag */
void TagEngineDtor()
{
  tag_engine_enabled = 0;
  tag_type = NULL;
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
  int i;

  assert(tag_type != NULL);

  EVP_MD_CTX_init(ctx);
  i = EVP_DigestInit_ex(ctx, tag_type, NULL);
  ZLOGIF(i != 1, "error initializing hash context");
  return i != 1 ? ERR_CODE : OK_CODE;
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
  EVP_MD_CTX_init(&tag);
  i = EVP_MD_CTX_copy_ex(&tag, ctx);
  ZLOGIF(i != 1, "error copying tag");
  EVP_DigestFinal_ex(&tag, p, &dsize);
  ZLOGIF(i != 1, "error finalizing tag");
  for(i = 0; i < dsize; ++i)
    sprintf(&digest[2*i], "%02X", p[i]);
}

/*
 * update etag with the given buffer.
 * returns 0 if all ok or -1 if failed
 */
int TagUpdate(void *ctx, const char *buffer, int32_t size)
{
  int i;

  assert(ctx != NULL);
  assert(buffer != NULL);
  assert(size >= 0);
  if(size == 0) return OK_CODE;

  /* update the context with a new data */
  i = EVP_DigestUpdate(ctx, buffer, size);
  ZLOGIF(i != 1, "error updating tag");
  return i != 1 ? ERR_CODE : OK_CODE;
}
