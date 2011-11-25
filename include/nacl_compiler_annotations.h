/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_SRC_INCLUDE_NACL_COMPILER_ANNOTATIONS_H_
#define NATIVE_CLIENT_SRC_INCLUDE_NACL_COMPILER_ANNOTATIONS_H_

/* MSVC supports "inline" only in C++ */
#if NACL_WINDOWS
# define INLINE __forceinline
#else
# define INLINE __inline__
#endif

#if NACL_WINDOWS
# define DLLEXPORT __declspec(dllexport)
#elif defined(NACL_LINUX) || defined(NACL_OSX)
# define DLLEXPORT __attribute__ ((visibility("default")))
#elif defined(__native_client__)
/* do nothing */
#else
# error "what platform?"
#endif

#if NACL_WINDOWS
# define ATTRIBUTE_FORMAT_PRINTF(m, n)
#else
# define ATTRIBUTE_FORMAT_PRINTF(m, n) __attribute__((format(printf, m, n)))
#endif

#if NACL_WINDOWS
# define UNREFERENCED_PARAMETER(P) (P)
#else
# define UNREFERENCED_PARAMETER(P) do { (void) P; } while (0)
#endif

#if NACL_WINDOWS
# define NORETURN __declspec(noreturn)
#else
# define NORETURN __attribute__((noreturn))  /* assumes gcc */
# define _cdecl /* empty */
#endif

#if NACL_WINDOWS
# define THREAD __declspec(thread)
# include <windows.h>
#else
# define THREAD __thread
# define WINAPI
#endif

#if NACL_WINDOWS
# define NACL_WUR
#else
# define NACL_WUR __attribute__((__warn_unused_result__))
#endif

#endif
