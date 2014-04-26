/*
 * untrusted boot (UBOOT) prototype
 *
 * WARNING: ro data and rw data wil be not loaded! only .text elf section
 * will be loaded by zerovm. so, do not use strings, static arrays e.t.c.
 *
 * WARNING: entry point should be 1st byte in .text section (zerovm will not
 * read entry point and will always assume it started from the .text beginning)
 *
 * WARNING: only zerovm api should be used
 */
#include "include/zvmlib.h"

void _start()
{
  /* find /boot/elf channel handle */
  /* read all */

  zvm_exit(12345);
}
