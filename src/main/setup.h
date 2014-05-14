/*
 * session setup and zerovm help screen
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

#ifndef SETUP_H__
#define SETUP_H__ 1

#include "src/main/manifest.h"

#define ZEROVM_PRIORITY 19
#define HELP_SCREEN /* update command line switches here */\
    "%s%s\033[1m\033[37mZeroVM 2 tag%d\033[0m lightweight VM manager, build 2014-05-14\n"\
    "Usage: <manifest> [-v#] [-T#] [-t#] [-s#] [-stFPQ]\n\n"\
    " -s <0..2> validation do/skip/skip twice\n"\
    " -t <0..3> report to stdout/log/fast (default 0)\n"\
    " -v <0..3> log verbosity (default 0)\n"\
    " -F quit right before starting user session\n"\
    " -P disable channels space preallocation\n"\
    " -Q disable platform qualification\n"\
    " -T enable trap calls tracing\n"

/*
 * wrapper. validate given buffer if it fits to nacl platform
 * return 0 if ok, 1 if failed
 */
int Validate(uint8_t* mbase, size_t size, uint32_t vbase);

/* the last frontier of defense. zerovm limits itself as much as possible */
void LastDefenseLine(struct Manifest *manifest);

/* session constructor. calls for all main constructors */
void SessionCtor(char *mft);

/* session destructor. calls for all main destructors, releases resources */
void SessionDtor(int code, char *state);

/* get system manifest */
struct Manifest *GetManifest();

#endif
