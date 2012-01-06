/*
 * Copyright 2008 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_

/*
 * macros to provide uniform access to identifiers from assembly due
 * to different C -> asm name mangling conventions and other platform-specific
 * requirements
 */
#define IDENTIFIER(n)  n
#define HIDDEN(n)  .hidden IDENTIFIER(n)
#define DEFINE_GLOBAL_HIDDEN_IDENTIFIER(n) \
  .globl IDENTIFIER(n); HIDDEN(n); IDENTIFIER(n)

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_ASM_H_ */
