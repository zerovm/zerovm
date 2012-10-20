/*
 * accounting of the user resources: local i/o, network i/o, cpu and memory
 *
 * AccountingCtor(nap)
 * AccountingDtor(nap)
 *
 *  Created on: Oct 14, 2012
 *      Author: d'b
 */
#include <assert.h>
#include <errno.h>
#include "src/service_runtime/sel_ldr.h"
#include "src/service_runtime/accounting.h"

/* accounting folder name. NULL if not available */
static char *acc_folder;

#if 0
/* read information for the given stat */
static int64_t ReadExtendedStat(const struct NaClApp *nap, const char *stat)
{
  char path[BIG_ENOUGH_SPACE + 1];
  char buf[BIG_ENOUGH_SPACE + 1];
  int64_t result;
  FILE *f;

  assert(nap != NULL);
  assert(stat != NULL);
  assert(acc_folder != NULL);

  /* construct the stat path to read */
  snprintf(path, BIG_ENOUGH_SPACE, "%s/%s", acc_folder, stat);

  /* open, read and close stat file */
  f = fopen(path, "r");
  if(f == NULL)
  {
    ZLOG(LOG_ERROR, "cannot open %s", path);
    return -1;
  }
  result = fread(buf, 1, BIG_ENOUGH_SPACE, f);
  if(result == 0 || result == BIG_ENOUGH_SPACE)
  {
    ZLOG(LOG_ERROR, "error statistics reading for %s", stat);
    return -1;
  }
  fclose(f);

  /* convert and return the result */
  return atoi(buf);
}

/*
 * populate given string with an extended accounting statistics
 * todo(d'b): under construction
 */
static void ReadExtendedAccounting(const struct NaClApp *nap, char *buf, int size)
{
  int64_t user_cpu = 0;
  int64_t memory_size = 0;
  int64_t swap_size = 0;
  int64_t real_cpu = 0;

  assert(nap != NULL);
  assert(buf != NULL);
  assert(nap->system_manifest != NULL);
  assert(size > 0);

  /* get statistics if there is an extended accounting */
  if(acc_folder != NULL)
  {
    /* get statistics */
    user_cpu = ReadExtendedStat(nap, CGROUPS_USER_CPU); /* ### cast to ms! */
    memory_size = ReadExtendedStat(nap, CGROUPS_MEMORY);
    swap_size = ReadExtendedStat(nap, CGROUPS_SWAP);
  }
  real_cpu = (int64_t)(clock() / (double)(CLOCKS_PER_SEC / 1000));

  /* construct and return the result */
  snprintf(buf, size, "%ld %ld %ld %ld", real_cpu, user_cpu, memory_size, swap_size);
}

/*
 * finalize an extended accounting
 * todo(d'b): under construction
 */
static void StopExtendedAccounting(struct NaClApp *nap)
{
  int result;

  assert(nap != NULL);
  assert(nap->system_manifest != NULL);

  /* exit if there is no extended accounting */
  if(acc_folder == NULL) return;

  /* todo(d'b): move itself to another "tasks" */

  /* remove own pid folder from cgroups folder */
  result = rmdir(acc_folder);
  if(result != 0)
  {
    ZLOG(LOG_ERROR, "cannot remove %s", acc_folder);
    return;
  }

  /* anything else?.. */
}
#endif

/* create/overwrite file and put integer in it */
static inline void EchoToFile(const char *path, int code)
{
  FILE *f = fopen(path, "w");

  ZLOGFAIL(f == NULL, errno, "cannot create file '%s'", path);
  fprintf(f, "%d", code);
  fclose(f);
}

/* initialize accounting */
void AccountingCtor(struct NaClApp *nap)
{
  struct stat st;
  char cfolder[BIG_ENOUGH_SPACE + 1];
  char counter[BIG_ENOUGH_SPACE + 1];
  int pid = (int32_t)getpid();
  int length;

  assert(nap != NULL);

  /* exit if the cgroups folder is missing */
  acc_folder = NULL;
  if(!(stat(CGROUPS_FOLDER, &st) == 0 && S_ISDIR(st.st_mode)))
    return;

  /* fail if folder of same pid exists and locked */
  length = snprintf(cfolder, BIG_ENOUGH_SPACE, "%s/%d", CGROUPS_FOLDER, pid);
  cfolder[BIG_ENOUGH_SPACE] = '\0';
  if(stat(cfolder, &st) == 0 && S_ISDIR(st.st_mode))
    ZLOGFAIL(rmdir(cfolder) == -1, errno, "'%s' in cgroups is already taken", cfolder);

  /* create folder of own pid */
  ZLOGFAIL(mkdir(cfolder, 0700) == -1, errno, "cannot create '%s' in cgroups", cfolder);

  /* store accounting folder to the system manifest */
  acc_folder = malloc(length + 1);
  ZLOGFAIL(acc_folder == NULL, ENOMEM,
      "cannot allocate memory to hold accounting folder name");
  strcpy(acc_folder, cfolder);

  /* create special file in it with own pid */
  snprintf(counter, BIG_ENOUGH_SPACE, "%s/%s", cfolder, CGROUPS_TASKS);
  EchoToFile(counter, pid);

  /* create user cpu accountant */
  snprintf(counter, BIG_ENOUGH_SPACE, "%s/%s", cfolder, CGROUPS_USER_CPU);
  EchoToFile(counter, 1);

  /* create memory accountant */
  snprintf(counter, BIG_ENOUGH_SPACE, "%s/%s", cfolder, CGROUPS_MEMORY);
  EchoToFile(counter, 1);

  /* create swap accountant */
  snprintf(counter, BIG_ENOUGH_SPACE, "%s/%s", cfolder, CGROUPS_SWAP);
  EchoToFile(counter, 1);
}

/* finalize accounting. return string with statistics */
void AccountingDtor(struct NaClApp *nap)
{
  assert(nap != NULL);

  nap->accounting = "disabled";
}
