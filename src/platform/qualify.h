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

/*
 * POSIX-specific routines for verifying that Data Execution Prevention is
 * functional.
 */

#ifndef QUALIFY_H_
#define QUALIFY_H_

EXTERN_C_BEGIN

/*
 * Runs the Platform Qualification tests required for safe sel startup.  This
 * may be a subset of the full set of PQ tests: it includes the tests that are
 * important enough to check at every startup, and tests that check aspects of
 * the system that may be subject to change.
 */
void RunSelQualificationTests();

EXTERN_C_END

#endif  /* QUALIFY_H_ */
