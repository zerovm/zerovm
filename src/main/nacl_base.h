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

/*
 * NaCl Basic Common Definitions.
 */
#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ 1

/*
 * putting extern "C" { } in header files make emacs want to indent
 * everything, which looks odd.  rather than putting in fancy syntax
 * recognition in c-mode, we just use the following macros.
 *
 * TODO: before releasing code, we should provide a defintion of a
 * function to be called from c-mode-hook that will make it easy to
 * follow our coding style (which we also need to document).
 */
#ifdef __cplusplus
# define EXTERN_C_BEGIN  extern "C" {
# define EXTERN_C_END    }

/* Mark this function as not throwing beyond */
#else
# define EXTERN_C_BEGIN
# define EXTERN_C_END
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ */
