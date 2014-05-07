/*
 * (re)store session engine
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

#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

/* load session from given image. 0: success, -1: failed */
int LoadSession(struct Manifest *manifest);

/* store session to image "Save". 0: success, -1: failed */
int SaveSession(struct Manifest *manifest);

#endif /* SNAPSHOT_H_ */
