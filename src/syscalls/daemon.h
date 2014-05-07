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
#ifndef DAEMON_H_
#define DAEMON_H_

/*
 * convert current session to daemon mode. after each new session spawning
 * this function will return to trap to serve the new session. if there is
 * no "Job" in manifest just continue current session. return user session
 * code (0: finalize session , -1: continue session)
 */
int Daemon(struct Manifest *manifest);

#endif /* DAEMON_H_ */
