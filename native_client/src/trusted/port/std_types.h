/*
 * Copyright 2010 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

#ifndef NATIVE_CLIENT_PORT_STD_TYPES_H_
#define NATIVE_CLIENT_PORT_STD_TYPES_H_

#if NACL_WINDOWS

// Disable warning for Windows "safe" vsprintf_s, strcpy_s, etc...
// since we use the same version for Linux/Mac.
#pragma warning(disable:4996)

// For intptr_t
#include <crtdefs.h>

typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#else
#include <stdint.h>
#endif

#endif  // NATIVE_CLIENT_PORT_STD_TYPES_H_

