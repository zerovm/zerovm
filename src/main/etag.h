/*
 * etag engine api
 *
 *  Created on: Sep 15, 2012
 *      Author: d'b
 */

#ifndef ETAG_H_
#define ETAG_H_

#include <stdint.h>

#define TAG_ENCRYPTION G_CHECKSUM_MD5
#define TAG_DIGEST_SIZE 32 + 1 /* tag digest size + '\0' (depend on TAG_ENCRYPTION) */
#define TAG_ENGINE_DISABLED "disabled"

/* etag engine construction */
void TagEngineCtor();

/* disable etag */
void TagEngineDtor();

/* return the etag_enabled state */
int TagEngineEnabled();

/*
 * initialize and return the hash context or abort if failed
 * to avoid memory leak context must be freed after usage
 */
void *TagCtor();

/* deallocate tag context */
void TagDtor(void *ctx);

/*
 * calculates digest from the context. can be used consequently
 * note: "digest" must have enough space to hold the digest
 */
void TagDigest(void *ctx, char *digest);

/*
 * update etag with the given buffer.
 * returns 0 if all ok or -1 if failed
 */
void TagUpdate(void *ctx, const char *buffer, int32_t size);

#endif /* ETAG_H_ */
