/*
 * zvm_netw.h
 *
 *  Created on: 03.06.2012
 *      Author: YaroslavLitvinov
 */

#ifndef ZVM_NETW_H_
#define ZVM_NETW_H_

#include <stdint.h>

#define PROTOID 0xABBA
#define PROTOVER 0x0001
#define DEFAULT_ZVM_NETW_HEAD {PROTOID, PROTOVER, 0, 0,0,0}

struct zvm_netw_header_t{
	uint16_t protoid;
	uint16_t protover;
	uint32_t req_len;
	uint64_t reserved1;
	uint64_t reserved2;
	uint64_t reserved3;
};

/*Only for unit-tests, it should not be used directly from zerovm code*/
struct db_records_t* db_records_forunittest();
struct zeromq_pool * zpool_forunittest();


/* dbname db path name
 * nodename param for sql query
 * nodeid param for postprocessing endpoints
 * return errcode EZVM..*/
int init_zvm_networking(const char *dbname, const char *nodename, int nodeid);
int term_zvm_networking();

/*Get capability for file, for unsupported return ENOTALLOWED*/
int
capabilities_for_file_fd(int fd);

/*stream write communication file*/
ssize_t commf_write(int fd, const char *buf, size_t count);
/*stream read communication file*/
ssize_t commf_read(int fd, char *buf, size_t count);


#endif /* ZVM_NETW_H_ */
