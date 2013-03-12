/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
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

#ifndef NACL_DEP_QUALIFY_H_
#define NACL_DEP_QUALIFY_H_

/*
 * Two of our target architectures (x86-64 and ARM) require that data not be
 * executable for our sandbox to hold.  Every vendor calls this something
 * different.  For the purposes of platform qualification, we use the term
 * 'data execution prevention,' or DEP.
 *
 * This file presents the common interface for DEP-check routines.  The
 * implementations differ greatly across platforms and architectures.  See the
 * notes on each function for details on where implementations live.
 */

#include <stddef.h>
#include "src/main/nacl_base.h"

EXTERN_C_BEGIN

/*
 * Checks that Data Execution Prevention is working as required by the
 * architecture.  On some architectures this is a no-op.
 */
int NaClCheckDEP();

EXTERN_C_END

#endif  /* NACL_DEP_QUALIFY_H_ */
