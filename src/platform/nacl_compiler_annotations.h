/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/*
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

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_COMPILER_ANNOTATIONS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_COMPILER_ANNOTATIONS_H_

/* gcc only */
#define INLINE __inline__
#define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
#define NORETURN __attribute__((noreturn))
#define NACL_WUR __attribute__((__warn_unused_result__))

#endif
