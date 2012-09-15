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

static int etag_enabled = 0;
static SHA_CTX overall_ctx;
static unsigned char overall_hash[SHA_DIGEST_LENGTH];

/* etag engine construction */
void EtagCtor()
{
  int code;

  etag_enabled = 1;
  code = SHA1_Init(&overall_ctx);
  COND_ABORT(code != 1, "error initializing etag engine");
}

/*
 * initialize the hash context in provided space
 * return 0 if everything is ok
 */
int ConstructCTX(SHA_CTX *ctx)
{
  int code;

  if(etag_enabled)
  {
    code = SHA1_Init(ctx);
    ErrIf(code == 0, "error initializing hash context");
    return code == 0 ? ERR_CODE : OK_CODE;
  }
  return ERR_CODE;
}

/*
 * accumulates hashes of all channels into the one digest
 * returns given string if updated or NULL when failed.
 * if NULL is given finalizes hash and return digest
 */
const unsigned char *OverallEtag(SHA_CTX *ctx)
{
  static unsigned char hash[SHA_DIGEST_LENGTH];
  int code;

  /* if etag disabled */
  if(etag_enabled == 0) return NULL;

  /* update overall_hash */
  if(ctx != NULL)
  {
    /* finalize local etag */
    code = SHA1_Final(hash, ctx);
    if(code == 0)
    {
      ErrIf(1, "local etag finalization error");
      return NULL;
    }
    code = SHA1_Update(&overall_ctx, hash, SHA_DIGEST_LENGTH);
  }
  /* finalize overall_hash */
  else
    code = SHA1_Final(overall_hash, &overall_ctx);

  ErrIf(code == 0, "error updating/finalizing overall hash");
  return ctx != NULL ? hash : overall_hash;
}

/*
 * make asciiz digest from the binary digest (must be 20 bytes array)
 * return asciiz digest or NULL if not available
 */
const char *EtagToText(unsigned char *p)
{
  static char hex[2 * SHA_DIGEST_LENGTH + 1] = {0};

  /* if etag disabled */
  if(etag_enabled == 0) return "disabled";

  assert(p != NULL);

  sprintf(hex, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
      "%02X%02X%02X%02X%02X", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8],
      p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19]);

  return hex;
}

/*
 * update etag with the given buffer. return the asciiz digest or NULL
 * if not available
 */
const char *UpdateEtag(SHA_CTX *etag, const char *buffer, int32_t size)
{
  unsigned char p[SHA_DIGEST_LENGTH];
  SHA_CTX tag;
  int code;

  /* if etag disabled */
  if(etag_enabled == 0) return NULL;

  assert(etag != NULL);
  assert(buffer != NULL);

  /* update channel tag (only with the really read data) */
  if(size > 0)
  {
    code = SHA1_Update(etag, buffer, size);
    ErrIf(code != 1, "error updating etag");
  }

  /* copy context aside and finalize it to get digest */
  memcpy(&tag, etag, sizeof tag);
  code = SHA1_Final(p, &tag);
  ErrIf(code != 1, "error finalizing etag");

  /* return asciiz digest */
  return EtagToText(p);
}
