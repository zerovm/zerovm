/*
 * first part of user memory manager. controls user space and call proxy
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

#ifndef USERSPACE_H_
#define USERSPACE_H_

/* allocate 84 gb of user space and call proxy */
void MakeUserSpace();

/* free 84gb of user space and call proxy */
void FreeUserSpace();

/*
 * initialize user memory with mandatory areas:
 * trampoline [RX], heap [RW], manifest [RX] and stack [RW]
 * note: channels should be already set
 */
void SetUserSpace();

#endif /* USERSPACE_H_ */
