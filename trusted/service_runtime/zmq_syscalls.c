#include <stdio.h>
#include <ctype.h>
#include <string.h> /* ### only need for strlen */
#include "trusted/service_runtime/zmq_syscalls.h"
#include "trusted/service_runtime/include/sys/errno.h"
#include "shared/platform/nacl_log.h"
#include "shared/platform/linux/nacl_host_desc_types.h"
#include "trusted/service_runtime/sel_ldr.h"
#include "trusted/desc/nacl_desc_io.h"
#include "trusted/service_runtime/nacl_app_thread.h"
#include "trusted/service_runtime/nacl_syscall_common.h"


//### remove it. mapping test
uint32_t SysMapInput(struct NaClAppThread *natp, void *buf);

/*
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

	return ((struct NaClDescIoDesc *) ndp)->hd->channel;
}

/*
 * check whether the given name is swift url
 * return 1 when "pathname" is swift url, otherwise - 0
 */
int IsSwiftURL(const char *name)
{
	char buf[] = URL_MARK;
	char *p = buf;

	if (name == NULL
	)
		return 0;

	while (*p++ == tolower(*name++))
		;

	/*###*/
	printf("returned by %s() BOOL_RESULT == %d\n", __func__, !(p - buf - URL_MARK_SIZE));

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

	//### test file mapping. remove it!
	// make special file record in the file descriptors table
	if(strcmp(name, "http://input.data") == 0)
	{
		hd->d = SysMapInput(natp, NULL);
	} else
	{ // "regular" zmq file
		/* open file and put file descriptor in hd */
		retval = NaClHostDescOpen(hd, name + URL_MARK_SIZE - 1, flags, mode);
	}

	/* add extra info to hd */
	strcpy(hd->channel, name);
	NaClLog(1, "NaClHostDescOpen(0x%08"NACL_PRIxPTR", %s, 0%o, 0%o) returned %d\n",
			(uintptr_t) hd, name + URL_MARK_SIZE - 1, flags, mode, retval);

	/* add a new record to the file descriptors table */
	if (0 == retval)
	{
		retval = NaClSetAvail(natp->nap, ((struct NaClDesc *) NaClDescIoDescMake(hd)));
		NaClLog(1, "Entered url into open file table at %d\n", retval);
	}

	/* ### log to remove. i only use it to debug this class */
	NaClLog(1, "int32_t ZMQSysOpen(struct NaClAppThread  *natp, char "
			"*pathname, int flags, int mode): channel == %s, desc == %d\n", hd->channel, hd->d);
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
int ZMQSysRead(struct NaClAppThread *natp, int d, void *buf, size_t count)
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

	// ### REMOVE IT FROM HERE
	// check if the file is not zmq but mapped
	if(strcmp(((struct NaClDescIoDesc *) ndp)->hd->channel, "http://input.data") == 0)
	{
		//##debug print
		printf("hello from %s!\n", __func__);

		// and return pointer to mapped file as "read bytes"
		return ((struct NaClDescIoDesc *) ndp)->hd->d;
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
int ZMQSysWrite(struct NaClAppThread *natp, int d, void *buf, size_t count)
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

	NaClLog(4, "In NaClSysWrite(%d, %.*s, %"NACL_PRIdS")\n", d, (int) count, (char *) sysaddr,
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

/*-----------------------------------------------------------
 * MAPPING METHODS NEED TO BE EXTRACTED.
 * temporary placement. also manifest will be rewritten and
 * get(key) shuoold be used instead of ugly loop
 */
/* put it out (put everything regarding file mapping to a new source)
 * map whole input file (from manifest) into memory.
 * return amount of read bytes when success, set given
 * pointer to start of mapped file otherwise - 0
 */
/*
 * syscalls cannot be used from inside of sandbox
 * because of wrong address space.
 */
#include "trusted/service_runtime/include/sys/mman.h"
#include "trusted/service_runtime/include/sys/stat.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

uint32_t SysMapInput(struct NaClAppThread *natp, void *buf)
{
	struct stat fs;
	int handle;
	int i = 0;
	int desc;
	char *name;
	struct NaClHostDesc *hd;

	NaClLog(3, "Entered SysMapInput()\n");
	buf = NULL;
	hd = malloc(sizeof *hd);
	if (NULL == hd) return -NACL_ABI_ENOMEM;

	/* get input file name for mapping */
	while(i < natp->nap->manifest_size &&
		strcmp(natp->nap->manifest[i].key, "input")) ++i;
	name = natp->nap->manifest[i].value;
	NaClLog(4, "SysMapInput() trying to open [%s] file\n", name);

	/* set all parameters and invoke syscall */
	handle = open(name, O_RDONLY); /* open file */
	if(handle < 0)
	  NaClLog(LOG_ERROR, "file \"%s\" could not open, err_code = %d\n", name, handle);
	NaClLog(4, "SysMapInput() file [%s] opened with %d handle\n", name, handle);

	i = fstat(handle, &fs); /* get file size */
	if(i < 0) NaClLog(LOG_ERROR, "file \"%s\" could get state, err_code = %d\n", name, i);

	hd->d = handle; /* construct nacl descriptor */
	desc = NaClSetAvail(natp->nap,
			((struct NaClDesc *) NaClDescIoDescMake(hd))); /* put to desc table and get index */

	/* map whole file into the memory */
	i = NaClCommonSysMmapIntern(natp->nap, NULL,
			(size_t)fs.st_size, NACL_ABI_PROT_READ | NACL_ABI_PROT_WRITE,
			NACL_ABI_MAP_SHARED, desc, 0);

	/* hack!! try to avoid it */
	//buf = (void*)((uintptr_t)buf & 0xffffffff); /* prevent sign flood */
	//## delete it!
	//printf("%lX\n", (uintptr_t)buf + natp->nap->mem_start);
	/*
	SHOWID;
	char *p;SHOWID;

	p = (char*)natp->nap->mem_start;SHOWID;

	printf("natp->nap->mem_start = %lX\n", natp->nap->mem_start);SHOWID;
	printf("p = %lX\n", (uintptr_t)p);SHOWID;
	printf("buf = %lX\n", (uintptr_t)buf);SHOWID;

	printf("p[0] = %c\n", p[0]);SHOWID;
	printf("p[1] = %c\n", p[1]);SHOWID;

	p	= buf + natp->nap->mem_start;SHOWID;

	printf("p[0] = %c\n", p[0]);SHOWID;
	printf("p[1] = %c\n", p[1]);SHOWID;

	p[10] = '\0';SHOWID;

	//printf("\n===============\n\n%s\n\n", (char*)((uintptr_t)buf + natp->nap->mem_start));
	printf("\n===============\n\n%s\n\n", p);
	*/

	/* free resources, check result and return */
	close(handle);
	printf("i = %x\n", i);
	buf = (void*)((uintptr_t)i);
	NaClLog(4, "SysMapInput(): mapped to buf = %lX\n", (uintptr_t)buf);

	/* the magic number is check for the largest nacl errno (i hope) */
	//return (uintptr_t)buf > (uintptr_t)0xFFFFF000 ? buf = NULL, 0 : fs.st_size;
	return (uint32_t)i;
}
