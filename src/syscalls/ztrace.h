/*
 * zerovm trace engine. can be used for simple profiling
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
#ifndef ZTRACE_H_
#define ZTRACE_H_

/* initialize "ztrace" service */
void ZTraceCtor();

/* close "ztrace" service. mode = 0 designed for "spawned" sessions */
void ZTraceDtor(int mode);

/* log the string with the next time delta */
void ZTrace(const char *msg);

/*
 * return the function name by its id
 * TODO(d'b): more neat solution need
 */
char *FunctionName(int id);

/* log syscall with the next time delta */
void SyscallZTrace(int id, int retcode, ...);

#endif /* ZTRACE_H_ */
