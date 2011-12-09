#include <stdio.h>
#include <ctype.h>
#include <string.h> /* ### only need for strlen */
#include "src/manifest/manifest_parser.h"
#include "src/manifest/manifest_setup.h"
#include "src/manifest/zmq_syscalls.h"
#include "src/service_runtime/include/sys/errno.h"
#include "src/platform/nacl_log.h"
#include "src/platform/linux/nacl_host_desc_types.h"
#include "src/service_runtime/sel_ldr.h"
#include "src/desc/nacl_desc_io.h"
#include "src/service_runtime/nacl_app_thread.h"
#include "src/service_runtime/nacl_syscall_common.h"

/* ###
 * helper function. extract file name from given
 * NaClAppThread and position in desc_tbl
 * return file name if successfully extracted, otherwise - NULL
 */
const char * GetFileName(struct NaClAppThread *natp, int d)
{
  struct NaClDesc *ndp;
  uintptr_t sysaddr;

  /* get channel name */
  ndp = NaClGetDesc(natp->nap, d);

  /* check if NaClDesc address is valid */
  sysaddr = NaClUserToSysAddr(natp->nap, (uintptr_t) ndp);
  if (kNaClBadAddress == sysaddr || NULL != ndp)
  {
    return NULL;
  }

//  return ((struct NaClDescIoDesc *) ndp)->hd->channel;
  return NULL; // ###
}

/*
 * check whether the given name is swift url
 * return 1 when "pathname" is swift url, otherwise - 0
 */
int IsSwiftURL(const char *name)
{
  char buf[] = URL_MARK;
  char *p = buf;

  if (name == NULL )
  return 0;

  while (*p++ == tolower(*name++)) ;

  return !(p - buf - URL_MARK_SIZE);
}

/* ###
 * get the fully qualified ip address from swift daemon
 * populate given NaClAppThread object with channel info
 * return 0 when success, otherwise - nacl error code
 */
int32_t ZMQSysOpen(struct NaClAppThread *natp, char *name, int flags, int mode)
{
	int32_t retval = 0;
	struct NaClHostDesc *hd;

	NaClLog(3, "Entered ZMQSysOpen(natp, %s, %X, %X)\n", name, flags, mode);

	/* ### dummy
	 * just open the local file w/o "http://" prefix to test this function
	 * example of file name: "http://dummy_zmq.dat" (will open "dummy_zmq.dat")
	 */
	hd = malloc(sizeof *hd);
	if (NULL == hd)
	{
		return -NACL_ABI_ENOMEM;
	}

	/* add extra info to hd */
//	strcpy(hd->channel, name); // ###
	NaClLog(1, "NaClHostDescOpen(0x%08"NACL_PRIxPTR", %s, 0%o, 0%o) returned %d\n",
			(uintptr_t) hd, name + URL_MARK_SIZE - 1, flags, mode, retval);

	/* add a new record to the file descriptors table */
	if (0 == retval)
	{
		retval = NaClSetAvail(natp->nap, ((struct NaClDesc *) NaClDescIoDescMake(hd)));
		NaClLog(1, "Entered url into open file table at %d\n", retval);
	}

	/* ### log to remove. i only use it to debug this class */
//	NaClLog(1, "int32_t ZMQSysOpen(struct NaClAppThread  *natp, char "
//			"*pathname, int flags, int mode): channel == %s, desc == %d\n", hd->channel, hd->d);
	return retval;
}

/* ###
 * ask the swift daemon to close channel
 * upadate NaClAppThread object with proper info
 * return 1 when success, otherwise - 0
 */
int ZMQSysClose(struct NaClAppThread *natp, int d)
{
	/* ###
	 * since this is just a dummy (file emulatung zmq channel)
	 * the procedure can just leave all the job to further nacl code
	 */

	/* ### log to remove. i only use it to debug this class */
	NaClLog(1, "int ZMQSysClose(struct NaClAppThread *natp, int d)\n");
	UNREFERENCED_PARAMETER(natp);
	UNREFERENCED_PARAMETER(d);
	return 1;
}

/* ###
 * get portion of data from opened channel (NaClAppThread object)
 * put (map) it to given memory region (NaClAppThread object)
 * return count of read bytes when success, otherwise - nacl error code
 */
int ZMQSysRead(struct NaClAppThread *natp, int d, void *buf, uint32_t count)
{
	ssize_t read_result;
	uintptr_t sysaddr;
	struct NaClDesc *ndp;

	/* ### log to remove. i only use it to debug this class */
	NaClLog(1, "int ZMQSysRead(struct NaClAppThread  *natp, int d, void *buf, size_t count) -- entered\n");

	ndp = NaClGetDesc(natp->nap, d);
	if (NULL == ndp)
		return -NACL_ABI_EINVAL;

	/* ###
	 * dummy code. just to test if class work proper
	 * delete it after zmq integration
	 */
	sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) buf, count);
	if (kNaClBadAddress == sysaddr)
	{
		NaClDescUnref(ndp);
		return -NACL_ABI_EFAULT;
	}

	/*
	 * The maximum length for read and write is INT32_MAX--anything larger and
	 * the return value would overflow. Passing larger values isn't an error--
	 * we'll just clamp the request size if it's too large.
	 */
	if (count > INT32_MAX)
	{
		count = INT32_MAX;
	}

	read_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->Read)
			(ndp, (void *) sysaddr,	count);
	if (read_result > 0)
	{
		NaClLog(4, "read returned %"NACL_PRIdS" bytes\n", read_result);
		NaClLog(8, "read result: %.*s\n", (int) read_result, (char *) sysaddr);
	}
	else
	{
		NaClLog(4, "read returned %"NACL_PRIdS"\n", read_result);
	}
	NaClDescUnref(ndp);

	/* This cast is safe because we clamped count above.*/
	return (int32_t) read_result;
}

/* ###
 * put (map) portion of data to opened channel (NaClAppThread object)
 * return count of written bytes when success, otherwise - nacl error code
 */
int ZMQSysWrite(struct NaClAppThread *natp, int d, void *buf, uint32_t count)
{
	ssize_t write_result;
	uintptr_t sysaddr;
	struct NaClDesc *ndp;

	/* ### log to remove. i only use it to debug this class */

	NaClLog(
			1,
			"int ZMQSysWrite(struct NaClAppThread *natp, int d, void *buf, size_t count) -- entered\n");

	ndp = NaClGetDesc(natp->nap, d);
	if (NULL == ndp)
		return -NACL_ABI_EINVAL;

	/* ###
	 * dummy code. just to test if class work proper
	 * delete it after zmq integration
	 */
	sysaddr = NaClUserToSysAddrRange(natp->nap, (uintptr_t) buf, count);
	if (kNaClBadAddress == sysaddr)
	{
		NaClDescUnref(ndp);
		return -NACL_ABI_EFAULT;
	}

	NaClLog(4, "In NaClSysWrite(%d, %.*s, %d)\n", d, (int) count, (char *) sysaddr,
			count);

	/*
	 * The maximum length for read and write is INT32_MAX--anything larger and
	 * the return value would overflow. Passing larger values isn't an error--
	 * we'll just clamp the request size if it's too large.
	 */
	if (count > INT32_MAX)
	{
		count = INT32_MAX;
	}

	write_result = (*((struct NaClDescVtbl const *) ndp->base.vtbl)->Write)(ndp, (void *) sysaddr,
			count);

	NaClDescUnref(ndp);

	/* This cast is safe because we clamped count above.*/
	return (int32_t) write_result;
}
