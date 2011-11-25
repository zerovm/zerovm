/*
 * Copyright 2010 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 *                 W   W   AA   RRR   N   N  III  N   N   GGG
 *                 W W W  A  A  R  R  NN  N   I   NN  N  G
 *                 W W W  AAAA  RRR   N N N   I   N N N  G GG
 *                 W W W  A  A  R R   N  NN   I   N  NN  G  G
 *                  W W   A  A  R  R  N   N  III  N   N   GGG
 *
 * DO NOT enable any options in this file in any production code/release.
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */

#ifndef NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_DANGEROUS_H_
#define NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_DANGEROUS_H_ 1

/*
 * NACL_DANGEROUS_STUFF_ENABLED is the master switch for dangerous
 * configuration options.  Only if it is enabled (set to non-zero)
 * should those configuration be possible.
 *
 * When adding other potentially dangerous configuration options, make
 * sure to follow the existing pattern.
 */
#define NACL_DANGEROUS_STUFF_ENABLED 0

#if 0 == NACL_DANGEROUS_STUFF_ENABLED
/*
 * Undefine the potentially dangerous options, so even if they were
 * present as compile-time defines the user will not be affected.  (We
 * control a warning message based on NACL_DANGEROUS_STUFF_ENABLED,
 * and would not want the warning to be missing when some dangerous
 * stuff are eabled.)
 *
 * The preprocessor treats non-macro identifiers as zero, so simply
 * undefining these works.  We avoid defining them to zero so that
 * programmers are not tempted to change them here rather than below.
 */
#  undef NACL_DANGEROUS_DEBUG_ONLY_NO_SANDBOX_RETURNS
#  undef NACL_DANGEROUS_DEBUG_MODE_DISABLE_INNER_SANDBOX
#else
/*
 * Enable NACL_DANGEROUS_DEBUG_ONLY_NO_SANDBOX_RETURNS only for
 * verifying that tests/syscall_return_sandboxing is working.  This is
 * intended to make it easier to test the validity of the test, used
 * primarily when bringing up a new architecture.
 */
#  if !defined(NACL_DANGEROUS_DEBUG_ONLY_NO_SANDBOX_RETURNS)
#    define NACL_DANGEROUS_DEBUG_ONLY_NO_SANDBOX_RETURNS    0
#  endif

/*
 * Enable NACL_DANGEROUS_DEBUG_MODE_DISABLE_INNER_SANDBOX only for
 * disabling the inner sandbox's checking of the start address for
 * bundle alignment, enabling the -I switch to disable ELF header
 * checks, and disabling the ARM thread pointer syscall return
 * sandboxing.
 */
#  if !defined(NACL_DANGEROUS_DEBUG_MODE_DISABLE_INNER_SANDBOX)
#    define NACL_DANGEROUS_DEBUG_MODE_DISABLE_INNER_SANDBOX 0
#  endif

#endif  /* NACL_DANGEROUS_STUFF_ENABLED */


#endif  /* NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME_NACL_CONFIG_DANGEROUS_H_ */
