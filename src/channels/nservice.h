/*
 * name service for network channels
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
#ifndef NSERVICE_H_
#define NSERVICE_H_

/*
 * TODO(d'b): find the neat solution to calculate (10915)
 * ((PARCEL_SIZE - sizeof(struct NSParcel)) / sizeof(struct NSRecord) + 1)
 */
#define MAX_CHANNELS_NUMBER 10915
#define MIN_CHANNELS_NUMBER 3

/* initialize name service and update channels information */
void NameServiceCtor(struct Manifest *manifest, uint32_t b, uint32_t c);

/* close name service */
void NameServiceDtor();

#endif
