/*
 * Copyright 2009 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */


// C/C++ library for handler passing in the Windows Chrome sandbox.
// sel_ldr side interface.

#ifndef NATIVE_CLIENT_SRC_TRUSTED_HANDLE_PASS_LDR_HANDLE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_HANDLE_PASS_LDR_HANDLE_H_

#if NACL_WINDOWS && !defined(NACL_STANDALONE)

#include <windows.h>
#include "native_client/src/include/nacl_base.h"
#include "native_client/src/trusted/desc/nacl_desc_base.h"

EXTERN_C_BEGIN

// Initialization function that needs to be called before the constructor.
void NaClHandlePassLdrInit();

// Construct the handle passing client.  Sets the socket address used to
// establish a connection to the lookup server in the browser plugin, and
// creates an SRPC client.  Returns 1 if connection was successful or 0
// otherwise.
extern int NaClHandlePassLdrCtor(struct NaClDesc* socket_address,
                                 DWORD renderer_pid,
                                 NaClHandle renderer_handle);

// Returns the HANDLE associated with the specified PID.  This may be
// returned from a local cache or by querying the lookup server in the
// browser plugin.  Both the current PID and the recipient PID must be
// known to the server for a successful lookup.
extern HANDLE NaClHandlePassLdrLookupHandle(DWORD recipient_pid);

// Destroy the handle passing client.
extern void NaClHandlePassLdrDtor();

EXTERN_C_END

#endif  // NACL_WINDOWS && !defined(NACL_STANDALONE)

#endif  // NATIVE_CLIENT_SRC_TRUSTED_HANDLE_PASS_LDR_HANDLE_H_
