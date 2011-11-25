/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_SRC_TRUSTED_REVERSE_SERVICE_MANIFEST_RPC_H_
#define NATIVE_CLIENT_SRC_TRUSTED_REVERSE_SERVICE_MANIFEST_RPC_H_

/*
 * Manifest RPC signatures, for use by C and C++ code.
 */

#define NACL_MANIFEST_LIST    "manifest_list::C"
#define NACL_MANIFEST_LOOKUP  "manifest_lookup:si:ihC"
#define NACL_MANIFEST_UNREF   "mainfest_unref:C:"

#endif
