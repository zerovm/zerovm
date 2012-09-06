/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * NaCl Service Runtime, C-level context switch code.
 */
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/nacl_globals.h"
#include "src/service_runtime/nacl_switch_to_app.h"

#define NORETURN_PTR NORETURN

static NORETURN_PTR void (*NaClSwitch)(struct NaClThreadContext *context);

void NaClInitSwitchToApp(struct NaClApp *nap) {
  NaClCPUFeaturesX86 cpu_features;

  UNREFERENCED_PARAMETER(nap);

  /*
   * TODO(mcgrathr): This call is repeated in platform qualification and
   * in every application of the validator.  It would be more efficient
   * to do it once and then reuse the same data.
   */
  NaClGetCurrentCPUFeatures(&cpu_features);
  if (NaClGetCPUFeature(&cpu_features, NaClCPUFeature_AVX)) {
    NaClSwitch = NaClSwitchAVX;
  } else {
    NaClSwitch = NaClSwitchSSE;
  }
}

/*
 * switch to the nacl module (untrusted content)
 */
NORETURN void NaClSwitchToApp(struct NaClApp *nap, nacl_reg_t new_prog_ctr)
{
  nacl_user->new_prog_ctr = new_prog_ctr;
  nacl_user->sysret = nap->sysret;
  NaClSwitch(nacl_user);
}
