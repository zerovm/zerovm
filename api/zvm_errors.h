/*
 * ZeroVM API error codes (only those whichuser can get).
 *
 * todo(d'b): trap() and nacl syscalls error codes should be gathered here
 *            there will be duplicates for nacl codes, how to solve it?
 *
 *  Created on: Jul 14, 2012
 *      Author: d'b
 */

#ifndef ZVM_ERRORS_H_
#define ZVM_ERRORS_H_

enum ZVM_CODES
{
  /* nacl syscall codes */

  /* trap syscall codes */
  ERR_CODE = -1, /* common error */
  OK_CODE = 0, /* must be zero */
  PARAM_CODE, /* invalid parameter */
  SIZE_CODE, /* invalid size */
  PTR_CODE, /* invalid pointer */
  SMALL_CODE, /* too small */
  LARGE_CODE, /* too large */

  INTERNAL_ERR = 256, /* codes bellow reserved by errno */
  INVALID_DESC = 257,
  INVALID_MODE = 258,
  INSANE_SIZE = 259,
  INSANE_OFFSET = 260,
  INVALID_BUFFER = 261,
  OUT_OF_BOUNDS = 262,
  OUT_OF_LIMITS = 263,
  INVALID_ERROR = 264
};

#endif /* ZVM_ERRORS_H_ */
