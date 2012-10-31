/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Returns 1 if the operating system can run Native Client modules.
 * d'b: since zerovm doesn't need shared memory support it always true
 */
int NaClOsIsSupported()
{
  return 1;
}
