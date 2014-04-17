/*
 * routines to calculate hashes. should be constructed from the command
 * line parser
 * update (2014.03.13): from now debug purposes only
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

#ifndef ETAG_H_
#define ETAG_H_

#include <stdint.h>
#include <glib.h>

/* compile time check if TAG_ENCRYPTION is specified and has sane value */
typedef char _1[TAG_ENCRYPTION];
typedef int _2[-(sizeof(_1) > G_CHECKSUM_SHA256)];

/* glib instead of macro have function returning digest size */
#define TAG_DIGEST_SIZE (8 * sizeof(_1) * sizeof(_1) + 32)

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

/* update etag with the given buffer */
void TagUpdate(void *ctx, const char *buffer, int64_t size);

#endif /* ETAG_H_ */
