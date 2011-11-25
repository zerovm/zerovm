/*
 * routines for passing zmq calls over the file operations
 * only "http://" prefix names will be processed
 */

#ifndef SERVICE_RUNTIME_ZMQ_SYSCALLS_H__
#define SERVICE_RUNTIME_ZMQ_SYSCALLS_H__ 1

#include "trusted/service_runtime/nacl_app_thread.h"

#define URL_MARK "http://"
#define URL_MARK_SIZE sizeof(URL_MARK)
#define SHOWID printf("\"%s\" %s() -- %d\n", __FILE__, __func__, __LINE__)

/*
 * helper function. extract channel name from given
 * NaClAppThread and position in desc_tbl
 * return channel name if successfully extracted, otherwise - NULL
 */
const char * GetFileName (struct NaClAppThread *natp, int d);

/*
 * check whether the given name is swift url
 * return 1 when "pathname" is swift url, otherwise - 0
 */
int IsSwiftURL(const char * pathname);

/* ###
 * get the fully qualified ip address from swift daemon
 * populate given NaClAppThread object with channel info
 * return file descriptor when success, otherwise -1
 */
int ZMQSysOpen();

/*
 * ask the swift daemon to close channel
 * upadate NaClAppThread object with proper info
 * return 1 when success, otherwise - 0
 */
int ZMQSysClose();

/*
 * get portion of data from opened channel (NaClAppThread object)
 * put (map) it to given memory region (NaClAppThread object)
 * return 1 when success, otherwise - 0
 */
int ZMQSysRead();

/*
 * put (map) portion of data to opened channel (NaClAppThread object)
 * return 1 when success, otherwise - 0
 */
int ZMQSysWrite();

#endif
