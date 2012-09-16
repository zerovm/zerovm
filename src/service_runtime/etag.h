/*
 * etag engine api
 *
 *  Created on: Sep 15, 2012
 *      Author: d'b
 */

#ifndef ETAG_H_
#define ETAG_H_

#include <stdint.h>
#include <openssl/sha.h>

#define ETAG_SIZE SHA_DIGEST_LENGTH * 2
#define ETAG_DISABLED "disabled"

/* etag engine construction */
void EtagCtor();

/* return the etag_enabled state */
int EtagEnabled();

/*
 * initialize the hash context in provided space
 * return 0 if everything is ok
 */
int ConstructCTX(SHA_CTX *ctx);

/*
 * accumulates hashes of all channels into the one digest
 * returns given string if updated or NULL when failed.
 * if NULL is given finalizes hash and return digest
 */
const unsigned char *OverallEtag(SHA_CTX *ctx);

/*
 * make asciiz digest from the binary digest (must be 20 bytes array)
 * return asciiz digest or NULL if not available
 */
const char *EtagToText(unsigned char *p);

/*
 * update etag with the given buffer. return the asciiz digest or NULL
 * if not available
 */
const char *UpdateEtag(SHA_CTX *etag, const char *buffer, int32_t size);

#endif /* ETAG_H_ */
