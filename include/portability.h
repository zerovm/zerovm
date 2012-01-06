/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This file should be at the top of the #include group, followed by
 * standard system #include files, then by native client specific
 * includes.
 *
 * TODO(gregoryd): explain why.  (Something to do with windows include
 * files, to be reconstructed.)
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_
#define NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ 1

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include "include/nacl_base.h"
#include "include/nacl_compiler_annotations.h"

/*
 * Per C99 7.8.14, define __STDC_CONSTANT_MACROS before including <stdint.h>
 * to get the INTn_C and UINTn_C macros for integer constants.  It's difficult
 * to guarantee any specific ordering of header includes, so it's difficult to
 * guarantee that the INTn_C macros can be defined by including <stdint.h> at
 * any specific point.  Provide GG_INTn_C macros instead.
 */
#define __WORDSIZE 64

#define GG_INT8_C(x)    (x)
#define GG_INT16_C(x)   (x)
#define GG_INT32_C(x)   (x)
#define GG_INT64_C(x)   GG_LONGLONG(x)

#define GG_UINT8_C(x)   (x ## U)
#define GG_UINT16_C(x)  (x ## U)
#define GG_UINT32_C(x)  (x ## U)
#define GG_UINT64_C(x)  GG_ULONGLONG(x)

#define GG_LONGLONG(x) x##LL
#define GG_ULONGLONG(x) x##ULL
# define LOCALTIME_R(in_time_t_ptr, out_struct_tm_ptr) \
  localtime_r(in_time_t_ptr, out_struct_tm_ptr)

/**
 * Processor architecture detection. This code was derived from
 * Chromium's build/build_config.h.
 * For more info on what's defined, see:
 * http://msdn.microsoft.com/en-us/library/b0084kay.aspx
 * http://www.agner.org/optimize/calling_conventions.pdf
 * r with gcc, run: "echo | gcc -E -dM -"
 */
#if defined(_M_X64) || defined(__x86_64__)
#define NACL_ARCH_CPU_X86_FAMILY 1
#define NACL_ARCH_CPU_X86_64 1
#define NACL_ARCH_CPU_64_BITS 1
#define NACL_HOST_WORDSIZE 64
#elif defined(_M_IX86) || defined(__i386__)
#define NACL_ARCH_CPU_X86_FAMILY 1
#define NACL_ARCH_CPU_X86 1
#define NACL_ARCH_CPU_32_BITS 1
#define NACL_HOST_WORDSIZE 32
#elif defined(__ARMEL__)
#define NACL_ARCH_CPU_ARM_FAMILY 1
#define NACL_ARCH_CPU_ARMEL 1
#define NACL_ARCH_CPU_32_BITS 1
#define NACL_HOST_WORDSIZE 32
#define NACL_WCHAR_T_IS_UNSIGNED 1
#elif defined(__pnacl__)
#define NACL_HOST_WORDSIZE 32
#else
#error Unrecognized host architecture
#endif

#ifndef SIZE_T_MAX
# define SIZE_T_MAX ((size_t) -1)
#endif

/* use uint64_t as largest integral type, assume 8 bit bytes */
#ifndef OFF_T_MIN
# define OFF_T_MIN ((off_t) (((uint64_t) 1) << (8 * sizeof(off_t) - 1)))
#endif
#ifndef OFF_T_MAX
# define OFF_T_MAX ((off_t) ~(((uint64_t) 1) << (8 * sizeof(off_t) - 1)))
#endif


/*
 * printf macros for size_t, in the style of inttypes.h.  this is
 * needed since the windows compiler does not understand %zd
 * etc. 64-bit windows uses 32-bit long and does not include long
 * long
 */
#define NACL___PRIS_PREFIX "l"

#if !defined(NACL_PRIdS)
#define NACL_PRIdS NACL___PRIS_PREFIX "d"
#endif
#if !defined(NACL_PRIiS)
#define NACL_PRIiS NACL___PRIS_PREFIX "i"
#endif
#if !defined(NACL_PRIoS)
#define NACL_PRIoS NACL___PRIS_PREFIX "o"
#endif
#if !defined (NACL_PRIuS)
#define NACL_PRIuS NACL___PRIS_PREFIX "u"
#endif
#if !defined(NACL_PRIxS)
#define NACL_PRIxS NACL___PRIS_PREFIX "x"
#endif
#if !defined(NACL_PRIXS)
#define NACL_PRIXS NACL___PRIS_PREFIX "X"
#endif

/*
 * printf macros for intptr_t and uintptr_t, int{8,16,32,64}
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS  /* C++ */
#endif

#include <inttypes.h>

#  define NACL_PRIdPTR PRIdPTR
#  define NACL_PRIiPTR PRIiPTR
#  define NACL_PRIoPTR PRIoPTR
#  define NACL_PRIuPTR PRIuPTR
#  define NACL_PRIxPTR PRIxPTR
#  define NACL_PRIXPTR PRIXPTR

# define NACL_PRId64 PRId64
# define NACL_PRIi64 PRIi64
# define NACL_PRIo64 PRIo64
# define NACL_PRIu64 PRIu64
# define NACL_PRIx64 PRIx64
# define NACL_PRIX64 PRIX64

# define NACL_PRId32 PRId32
# define NACL_PRIi32 PRIi32
# define NACL_PRIo32 PRIo32
# define NACL_PRIu32 PRIu32
# define NACL_PRIx32 PRIx32
# define NACL_PRIX32 PRIX32

# define NACL_PRId16 PRId16
# define NACL_PRIi16 PRIi16
# define NACL_PRIo16 PRIo16
# define NACL_PRIu16 PRIu16
# define NACL_PRIx16 PRIx16
# define NACL_PRIX16 PRIX16

# define NACL_PRId8 PRId8
# define NACL_PRIi8 PRIi8
# define NACL_PRIo8 PRIo8
# define NACL_PRIu8 PRIu8
# define NACL_PRIx8 PRIx8
# define NACL_PRIX8 PRIX8

/*
 * macros for run-time error detectors (such as Valgrind/Memcheck).
 */
#if defined(_DEBUG) && NACL_LINUX
#include "third_party/valgrind/memcheck.h"
#define NACL_MAKE_MEM_UNDEFINED(a, b) (void) VALGRIND_MAKE_MEM_UNDEFINED(a, b)
#else
#define NACL_MAKE_MEM_UNDEFINED(a, b)
#endif

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ */
