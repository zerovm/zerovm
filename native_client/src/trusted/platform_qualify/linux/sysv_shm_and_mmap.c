/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * This test verifies that SysV shared memory, created via shmget and
 * mapped via shmat, can be mmap'd over in parts.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include "native_client/src/trusted/platform_qualify/linux/sysv_shm_and_mmap.h"

#define SYSVSHM_SIZE  65536
#define MMAP_OFFSET   4096
#define MMAP_SIZE     4096

#ifdef DEBUG_SHM_AND_MMAP
# define DPRINTF(arglist) do { printf arglist; } while (0)
#else
# define DPRINTF(arglist) do { ; } while (0)
#endif

static void FillWithPattern(void   *memory,
                            size_t size,
                            int    counter) {
  unsigned char *mem_ptr = (unsigned char *) memory;
  size_t        ix;

  for (ix = 0; ix < size; ++ix) {
    mem_ptr[ix] = counter;
    ++counter;
    counter &= 0xff;
  }
}

static int VerifyPattern(void   *memory,
                         size_t size,
                         int    counter) {
  unsigned char *mem_ptr = (unsigned char *) memory;
  size_t        ix;

  for (ix = 0; ix < size; ++ix) {
    if (mem_ptr[ix] != counter) {
      fprintf(stderr,
              "Memory at %p is wrong: expected %d, got %d\n",
              mem_ptr + ix,
              counter,
              mem_ptr[ix]);
      return 0;
    }
    ++counter;
    counter &= 0xff;
  }
  return 1;
}


/*
 * Verify the allocated shared memory ID.
 */
int NaClPlatformQualifySysVShmId(int shm_id) {
  void            *shm_addr;
  void            *shm_addr2;
  void            *mmap_addr;
  struct shmid_ds shm_ds;

  shm_addr = shmat(shm_id, (const void *) NULL, 0);
  DPRINTF(("shmat -> %p\n", shm_addr));
  if (NULL == shm_addr) {
    perror("platform_qualify: sysv_shm_and_mmap: shmat");
    return 2;
  }

  FillWithPattern(shm_addr, SYSVSHM_SIZE, 0);

  if (-1 == shmctl(shm_id, IPC_STAT, &shm_ds)) {
    perror("platform_qualify: sysv_shm_and_mmap: no shmctl IPC_STAT\n");
    return 3;
  }
  if (1 != shm_ds.shm_nattch) {
    fprintf(stderr,
            "platform_qualify: sysv_shm_and_mmap: shm_nattch (%lu) != 1\n",
            (unsigned long) shm_ds.shm_nattch);
    return 4;
  }

  mmap_addr = (void *) ((uintptr_t) shm_addr + MMAP_OFFSET);
  DPRINTF(("mmap addr %p\n", mmap_addr));
  if (MAP_FAILED == mmap(mmap_addr,
                         MMAP_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                         -1,
                         0)) {
    perror("platform_qualify: sysv_shm_and_mmap: mmap");
    return 5;
  }
  DPRINTF(("mmap succeeded\n"));
  /*
   * Splitting an shmat mapping will increase the shm_nattch count by 1.
   */

  if (-1 == shmctl(shm_id, IPC_STAT, &shm_ds)) {
    perror("platform_qualify: sysv_shm_and_mmap: no shmctl IPC_STAT\n");
    return 6;
  }
  if (2 != shm_ds.shm_nattch) {
    fprintf(stderr,
            "platform_qualify: sysv_shm_and_mmap: shm_nattch (%lu) != 2\n",
           (unsigned long) shm_ds.shm_nattch);
    return 7;
  }

  FillWithPattern(mmap_addr, MMAP_SIZE, 1);

  shm_addr2 = shmat(shm_id, (const void *) NULL, 0);
  DPRINTF(("shmat -> %p\n", shm_addr2));
  if (NULL == shm_addr2) {
    perror("platform_qualify: sysv_shm_and_mmap: shmat 2nd time\n");
    return 8;
  }

  if (-1 == shmctl(shm_id, IPC_STAT, &shm_ds)) {
    perror("platform_qualify: sysv_shm_and_mmap: no shmctl IPC_STAT\n");
    return 9;
  }
  if (3 != shm_ds.shm_nattch) {
    fprintf(stderr,
            "platform_qualify: sysv_shm_and_mmap: shm_nattch (%lu) != 3\n",
           (unsigned long) shm_ds.shm_nattch);
    return 10;
  }

  if (!VerifyPattern(shm_addr2, SYSVSHM_SIZE, 0)) {
    fprintf(stderr,
            "platform_qualify: writing to mmap memory overwrote"
            " sysv shm memory?!?\n");
    return 11;
  }
  FillWithPattern(shm_addr2, SYSVSHM_SIZE, 0);
  if (!VerifyPattern(mmap_addr, MMAP_SIZE, 1)) {
    fprintf(stderr,
            "platform_qualify: writng to shm memory"
            " overwrite mmap memory?!?\n");
    return 12;
  }
  if (MAP_FAILED == mmap(shm_addr,
                         SYSVSHM_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                         -1,
                         0)) {
    perror("platform_qualify: sysv_shm_and_mmap: mmap over all"
           " shm_addr failed\n");
    return 13;
  }

  if (-1 == shmctl(shm_id, IPC_STAT, &shm_ds)) {
    perror("platform_qualify: sysv_shm_and_mmap: no shmctl IPC_STAT\n");
    return 14;
  }
  if (1 != shm_ds.shm_nattch) {
    fprintf(stderr,
            "platform_qualify: sysv_shm_and_mmap: shm_nattch (%lu) != 1\n",
           (unsigned long) shm_ds.shm_nattch);
    return 15;
  }

  if (MAP_FAILED == mmap(shm_addr2,
                         SYSVSHM_SIZE,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                         -1,
                         0)) {
    perror("platform_qualify: sysv_shm_and_mmap: mmap over all"
           " shm_addr2 failed\n");
    return 16;
  }
  if (-1 == shmctl(shm_id, IPC_STAT, &shm_ds)) {
    perror("platform_qualify: sysv_shm_and_mmap: no shmctl IPC_STAT\n");
    return 17;
  }
  if (0 != shm_ds.shm_nattch) {
    fprintf(stderr,
            "platform_qualify: sysv_shm_and_mmap: over-mmap'd shm does not"
            " reduce shm_nattch (%lu)\n",
            (unsigned long) shm_ds.shm_nattch);
    return 18;
  }

  return 0;
 }

/*
 * If this code is interrupted or for some reason segvs, then the
 * shared memory is leaked.  In that case, use "ipcs -m" to look for
 * segments with key of 0 and bytes of 65536, and then use "ipcrm -m
 * shmid" for the corresponding shmid number to garbage collect the
 * shared memory segment.
 */


/*
 * To build as a standalone, build with -DSysVShmAndMmapHasProblems=main.
 */
int NaClPlatformQualifySysVShmAndMmapHasProblems(void) {
  int err_code = 0;
  int shm_id = -1;

  shm_id = shmget(IPC_PRIVATE,
                  SYSVSHM_SIZE,
                  IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  DPRINTF(("shmget -> %d\n", shm_id));

  if (-1 == shm_id) {
    perror("platform_qualify: sysv_shm_and_mmap: shmget");
    return 1;
  }

  err_code = NaClPlatformQualifySysVShmId(shm_id);

  if (-1 == shmctl(shm_id, IPC_RMID, NULL)) {
    perror("platform_qualify: sysv_shm_and_mmap: shmctl IPC_RMID failed\n");

    /*
     * Set the err code to a release failure only if we do not already have
     * a more interesting failure to report.
     */
    if (!err_code) err_code = 19;
  }

  return err_code;
}
