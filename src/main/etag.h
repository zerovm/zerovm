/*
 * etag engine api
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

#define TAG_ENCRYPTION G_CHECKSUM_SHA1 /* G_CHECKSUM_MD5, G_CHECKSUM_SHA1, G_CHECKSUM_SHA256 */
#define TAG_DIGEST_SIZE 40 + 1 /* without '\0': 32, 40, 64 */
#define TAG_ENGINE_DISABLED "disabled"

/*
 * calculate by mask. return 0 if disabled
 * 00000001 - channels etag enabled
 * 00000010 - memory etag enabled
 */
#define CHANNELS_ETAG_ENABLED (TagEngineEnabled() & 1)
#define MEMORY_ETAG_ENABLED (TagEngineEnabled() & 2)

/* etag engine construction */
void TagEngineCtor(int level);

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
void TagUpdate(void *ctx, const char *buffer, int64_t size);

#endif /* ETAG_H_ */
