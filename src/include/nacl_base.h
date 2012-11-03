/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Basic Common Definitions.
 */
#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ 1

/*
 * putting extern "C" { } in header files make emacs want to indent
 * everything, which looks odd.  rather than putting in fancy syntax
 * recognition in c-mode, we just use the following macros.
 *
 * TODO: before releasing code, we should provide a defintion of a
 * function to be called from c-mode-hook that will make it easy to
 * follow our coding style (which we also need to document).
 */
#ifdef __cplusplus
# define EXTERN_C_BEGIN  extern "C" {
# define EXTERN_C_END    }

/* Mark this function as not throwing beyond */
#else
# define EXTERN_C_BEGIN
# define EXTERN_C_END
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_NACL_BASE_H_ */
