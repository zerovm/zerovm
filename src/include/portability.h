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

#include "src/include/nacl_base.h"
#include "src/include/nacl_compiler_annotations.h"

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
#define LOCALTIME_R(in_time_t_ptr, out_struct_tm_ptr) \
  localtime_r(in_time_t_ptr, out_struct_tm_ptr)

/* zerovm only uses linux x86 64 platform */
#define NACL_ARCH_CPU_X86_FAMILY 1
#define NACL_ARCH_CPU_X86_64 1
#define NACL_ARCH_CPU_64_BITS 1
#define NACL_HOST_WORDSIZE 64

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

/* printf macros for intptr_t and uintptr_t, int{8,16,32,64} */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS  /* C++ */
#endif

#define SNPRINTF snprintf

#include <inttypes.h>

/* macros for run-time error detectors (such as Valgrind/Memcheck) */
#define NACL_MAKE_MEM_UNDEFINED(a, b)

#endif  /* NATIVE_CLIENT_SRC_INCLUDE_PORTABILITY_H_ */
