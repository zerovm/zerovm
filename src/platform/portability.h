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
 * This file should be at the top of the #include group, followed by
 * standard system #include files, then by native client specific
 * includes.
 *
 * TODO(gregoryd): explain why.  (Something to do with windows include
 * files, to be reconstructed.)
 */

#ifndef PORTABILITY_H_
#define PORTABILITY_H_ 1

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "src/main/nacl_base.h"

#ifndef SIZE_T_MAX
# define SIZE_T_MAX ((size_t) -1)
#endif

#include <inttypes.h>

#endif  /* PORTABILITY_H_ */
