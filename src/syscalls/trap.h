/*
 * "single syscall" engine. this is the replacement for nacl multiple syscalls
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

#ifndef TRAP_H_
#define TRAP_H_

EXTERN_C_BEGIN

/*
 * 1st parameter is a pointer to the command (function, arg1, argv2,..)
 * 2nd parameter is a pointer to return value(s)
 * return 0 if successful, otherwise -errno
 *
 * notice about args: since nacl patches two 1st arguments if they are pointers,
 * arg[1] should not be used
 */
int32_t TrapHandler(uint32_t args);

EXTERN_C_END

#endif /* TRAP_H_ */
