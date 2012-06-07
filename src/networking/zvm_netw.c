/*
 * zvm_netw.c
 *
 *  Created on: 03.06.2012
 *      Author: YaroslavLitvinov
 */


#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "src/platform/nacl_log.h"
#include "src/networking/zmq_netw.h"
#include "src/networking/zvm_netw.h"
#include "src/networking/sqluse_srv.h"
#include "src/networking/errcodes.h"
#include "zmq.h"

static struct db_records_t* __db_records = NULL;
static struct zeromq_pool * __zpool = NULL;

struct db_records_t* db_records_forunittest(){ return __db_records; }
struct zeromq_pool * zpool_forunittest(){ return __zpool;}

static uint64_t get_file_size(const char *name)
{
  struct stat fs;
  int handle;
  int i;

  handle = open(name, O_RDONLY);
  if(handle < 0) return -1;

  i = fstat(handle, &fs);
  close(handle);
  return i < 0 ? -1 : fs.st_size;
}

int
capabilities_for_file_fd(int fd){
	if ( __db_records ){
		struct db_record_t* db_record = match_db_record_by_fd( __db_records, fd);
		if ( db_record ){
			if ( 'r' == db_record->fmode )
				return EREAD;
			else if ( 'w' == db_record->fmode )
				return EWRITE;
		}
		else{
			NaClLog(LOG_ERROR, "%s() db_record is NULL for specified fd=%d\n", __func__, fd );
		}
	}
	else{
		NaClLog(LOG_ERROR, "%s() __db_records NULL, not properly configured\n", __func__);
	}
	return ENOTALLOWED;
}


int init_zvm_networking(const char *dbname, const char *nodename, int nodeid){
	uint64_t db_size = 0;

	if ( !nodename || !dbname || nodeid<0 ) return EZVMBAD_ARG;
	__db_records = malloc(sizeof(struct db_records_t));
	memset(__db_records, '\0', sizeof(struct db_records_t));

	__db_records->cid = nodeid;
	db_size = get_file_size(dbname);
	if ( -1 != db_size && 0 != db_size ){
		int err = 0;
		NaClLog(LOG_INFO, "reading database = %s, cid=%d, nodename=%s\n", dbname, nodeid, nodename);
		err = get_all_records_from_dbtable(dbname, nodename, __db_records);
		if ( err ){
			NaClLog(LOG_ERROR, "database %s read error= %d\n", dbname, err);
			return EZVM_DBREADERR;
		}else{
			__zpool = malloc(sizeof(struct zeromq_pool));
			init_zeromq_pool(__zpool);
			if ( ERR_OK != open_all_comm_files(__zpool, __db_records) )
				return EZVM_SOCK_ERROR;
		}
	}
	else{
		NaClLog(LOG_ERROR, "NETWORKING defined, db does not exist or empty\n");
		return EZVMDBNOTEXIST_OREMPTY;
	}
	return EZVM_OK;
}


int term_zvm_networking(){
	int err = 0;
	if ( !__zpool || !__db_records ) return EZVM_NOT_INITED;
	NaClLog(LOG_INFO, "close_all_comm_files");
	if ( ERR_OK != close_all_comm_files(__zpool) ){
		NaClLog(LOG_ERROR, "close_all_comm_files err=%d", err);
		return EZVM_SOCK_ERROR;
	}
	else{
		NaClLog(LOG_INFO, "close_all_comm_files OK");
		return EZVM_OK;
	}
}


ssize_t commf_read(int fd, char *buf, size_t count){
	struct sock_file_t* sockf = NULL;
	int capab = capabilities_for_file_fd(fd);
	ssize_t read_bytes = -1;
	if ( !__zpool ) return -1;
	if ( EREAD != capab && EREADWRITE != capab ) return -1;

	sockf = sockf_by_fd(__zpool, fd);
	if ( sockf ){
		if ( sockf->sock_type == ESOCKET_REQREP ){
			struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
			NaClLog(LOG_ERROR, "%s() attempt read from fd=%d, count=%d\n", __func__, fd, (int)count );
			/*use socket by REQ-REP pattern. Should be used in next way:
			 * write request HEADER
			 * read  response data*/
			header.req_len = count;
			//request data size we want read
			NaClLog(LOG_ERROR, "%s() write fd=%d header size=%d\n", __func__, fd, (int)sizeof(header) );
			assert( sizeof(header) == write_sockf(sockf, (const char*)&header, sizeof(header) ) );
			//read requested data
			read_bytes = read_sockf(sockf, buf, count);
			NaClLog(LOG_ERROR, "%s() read ok from fd=%d, requested%d, readed=%d\n", __func__, fd, (int)count, (int)read_bytes );
		}
		else{
			NaClLog(LOG_ERROR, "%s() for fd=%d, unsupported socket type=%d\n", __func__, fd, sockf->sock_type );
		}
	}
	else{
		NaClLog(LOG_ERROR, "%s() for given fd=%d, socket NULL\n", __func__, fd );
	}
	return read_bytes;
}


ssize_t commf_write(int fd, const char *buf, size_t count){
	struct sock_file_t* sockf = NULL;
	int capab = capabilities_for_file_fd(fd);
	ssize_t wrote_bytes = -1;
	if ( !__zpool ) return -1;
	if ( EWRITE != capab && EREADWRITE != capab ) return -1;

	sockf = sockf_by_fd(__zpool, fd);
	if ( sockf ){
		if ( sockf->sock_type == ESOCKET_REQREP ){
			size_t sdata = 0;
			/*use socket by REQ-REP pattern. Should be used in next way:
			 * read request HEADER
			 * write requested data*/
			struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
			NaClLog(LOG_ERROR, "%s() attempt write to fd=%d, count=%d\n", __func__, fd, (int)count );
			//read request for data
			assert( sizeof(header) == read_sockf(sockf, (char*)&header, sizeof(header)));
			assert(header.protoid == PROTOID);
			//write only requested data from header.len, or all data if user specifies less
			sdata = min(header.req_len, count);
			assert( sdata == write_sockf(sockf, buf, sdata ) );
			wrote_bytes = sdata;
			NaClLog(LOG_ERROR, "%s() read ok from fd=%d, requested%d, readed=%d\n", __func__, fd, (int)count, (int)wrote_bytes );
		}
		else{
			NaClLog(LOG_ERROR, "%s() for fd=%d, unsupported socket type=%d\n", __func__, fd, sockf->sock_type );
		}
	}
	else{
		NaClLog(LOG_ERROR, "%s() for given fd=%d, socket NULL\n", __func__, fd );
	}
	return wrote_bytes;
}



