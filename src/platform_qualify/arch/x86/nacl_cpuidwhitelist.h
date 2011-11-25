/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_X86_NACL_CPUIDWHITELIST
#define NATIVE_CLIENT_SRC_TRUSTED_PLATFORM_QUALIFY_ARCH_X86_NACL_CPUIDWHITELIST

extern Bool NaCl_CPUIsWhitelisted(const char *myid);
extern Bool NaCl_CPUIsBlacklisted(const char *myid);

#endif
