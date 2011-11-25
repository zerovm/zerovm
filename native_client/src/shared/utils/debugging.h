/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

/* Define simple debugging utilities that are turned on/off by the
 * value of the define flag DEBUGGING.
 *
 * To turn on debugging of instruction decoding, change value of
 * DEBUGGING to 1.
 *
 */

#ifndef NATIVE_CLIENT_SRC_SHARED_UTILS_DEBUGGING_H__
#define NATIVE_CLIENT_SRC_SHARED_UTILS_DEBUGGING_H__

/* Turn off debugging if not otherwise specified in the specific code file. */
#ifndef DEBUGGING
#define DEBUGGING 0
#endif

#if DEBUGGING
/* Defines to execute statement(s) s if in DEBUGGING mode, and compile
 * in either mode. This allows type checking to be applied at all times.
 */
#define DEBUG(s) s
#else
/* Defines to compile but not include statement(s) s if not
 * in DEBUGGING mode.
 */
#define DEBUG(s) do { if (0) { s; } } while (0)
#endif

#if DEBUGGING
/* Defines to compile execute statement(s) if in DEBUGGING mode.
 * Erases otherwise. This should only be used when linking
 * would otherwise require the symbol when DEBUGGING is false.
*/
#define DEBUG_OR_ERASE(s) s
#else
/* Defines to erase s if not in DEBUGGING mode. */
#define DEBUG_OR_ERASE(s)
#endif

#endif  /* NATIVE_CLIENT_SRC_SHARED_UTILS_DEBUGGING_H__ */
