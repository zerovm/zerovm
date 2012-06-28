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
#include <zmq.h>

#include "src/utils/tools.h"
#include "src/service_runtime/nacl_error_code.h"
#include "src/service_runtime/dyn_array.h"
#include "src/manifest/mount_channel.h"
#include "src/platform/nacl_log.h"

#include "src/networking/zmq_netw.h"
#include "src/networking/zvm_netw.h"
#include "src/networking/sqluse_srv.h"

static struct db_records_t* __db_records = NULL;
static struct zeromq_pool * __zpool = NULL;
static struct zmq_netw_interface *__netw_if = NULL;

/*Do not allow zeromq objective code just here for unit tests*/
#ifndef UNIT_TEST

/*
 * todo(d'b): change c99 initialization to c89
 */
/*Initialization I/O interface by zeromq functions. Should be used only for interface initialization*/
static struct zmq_netw_interface __zmq_netw_implementation = {
	.get_all_dbrecords       = get_all_records_from_dbtable,
	.match_db_record_by_fd   = match_db_record_by_fd,
	.GetFileSize             = GetFileSize,
	.sockf_by_fd             = sockf_by_fd,
	.init_zeromq_pool        = init_zeromq_pool,
	.open_all_comm_files     = open_all_comm_files,
	.close_all_comm_files    = close_all_comm_files,
	.zeromq_term             = zeromq_term,
	.write_sockf             = write_sockf,
	.read_sockf              = read_sockf
};
struct zmq_netw_interface* zmq_netw_interface_implementation(){
	return &__zmq_netw_implementation;
}
#else
	/*stub instead zeromq interface*/
	struct zeromq_interface* zeromq_interface_implementation(){ return NULL; }
	struct zeromq_pool *pool_for_unit_test(){ return __zpool; }
	struct db_records_t *db_records_for_unit_test(){ return __db_records; }
#endif


int
capabilities_for_file_fd(int fd){
	if ( __db_records ){
		struct db_record_t* db_record = __netw_if->match_db_record_by_fd( __db_records, fd);
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
	return ENOTALLOWED;
}


int init_zvm_networking(struct zmq_netw_interface *netw_if, const char *dbname, const char *nodename, int nodeid){
	uint64_t db_size = 0;
	int err = LOAD_OK;
	assert(nodename);
	assert(dbname);
	assert(nodeid);

	__netw_if = netw_if;
	__db_records = malloc(sizeof(struct db_records_t));
	memset(__db_records, '\0', sizeof(struct db_records_t));
	__db_records->cid = nodeid;

	db_size = __netw_if->GetFileSize(dbname);
	if ( -1 != db_size && 0 != db_size ){
		NaClLog(LOG_INFO, "reading database = %s, cid=%d, nodename=%s\n", dbname, nodeid, nodename);
		err = __netw_if->get_all_dbrecords(dbname, nodename, __db_records);
		if ( err != LOAD_OK ){
			return err;
		}else{
			__zpool = malloc(sizeof(struct zeromq_pool));
			assert(__zpool != NULL);
			/*For generic zmq_netw using init interface by zeromq implementation*/
			err = __netw_if->init_zeromq_pool( zeromq_interface_implementation(), __zpool );
			if ( err != LOAD_OK ){
				return err;
			}
			else{
				err = __netw_if->open_all_comm_files(__zpool, __db_records);
				if ( err != LOAD_OK )
					return err;
				}
		}
	}
	else{
		NaClLog(LOG_ERROR, "NETWORKING defined, db does not exist or empty\n");
		return LOAD_OPEN_ERROR;
	}
	return LOAD_OK;
}


int term_zvm_networking(){
	int err = LOAD_OK;
	int applicative_err = LOAD_OK;
	struct sock_file_t *sockf = NULL;

	/*write all FIN requests*/
	for (int i=0; i < __zpool->sockf_array->num_entries; i++){
		sockf = __zpool->sockf_array->ptr_array[i];
		if ( sockf && ESOCKET_REQREP == sockf->sock_type ){
			if ( 'r' == sockf->access_mode ){
				/*For r socket: write FIN request*/
				struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
				header.command = COMMAND_FIN;
				NaClLog(LOG_ERROR, "%s() write FIN request to fd=%d\n", __func__, sockf->fs_fd );
				//write FIN request
				assert(header.protoid == PROTOID);
				assert(header.command == COMMAND_FIN);
				assert( sizeof(header) == __netw_if->write_sockf(sockf, (char*)&header, sizeof(header)));

			}
		}
	}

	/*Read all FIN requests*/
	for (int i=0; i < __zpool->sockf_array->num_entries; i++){
		sockf = __zpool->sockf_array->ptr_array[i];
		if ( sockf && ESOCKET_REQREP == sockf->sock_type ){
			if ( 'w' == sockf->access_mode ){
				/*For r socket: write FIN request*/
				struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
				NaClLog(LOG_ERROR, "%s() read FIN request from fd=%d\n", __func__, sockf->fs_fd );
				//read request for data
				assert( sizeof(header) == __netw_if->read_sockf(sockf, (char*)&header, sizeof(header)));
				assert(header.protoid == PROTOID);
				if ( header.command != COMMAND_FIN ){
					applicative_err = LOAD_APPLICATIVE_NETWORK_ERROR;
				}
			}
		}
	}

	err = __netw_if->close_all_comm_files(__zpool);
	if ( err != LOAD_OK  ){
		NaClLog(LOG_ERROR, "close_all_comm_files error=%d", err);
		return err;
	}
	err = __netw_if->zeromq_term(__zpool);
	if ( err != LOAD_OK ){
		NaClLog(LOG_ERROR, "zeromq_term error=%d", err);
		return err;
	}

	for (int i=0; i < __db_records->count; i++){
		free(__db_records->array[i].endpoint), __db_records->array[i].endpoint = NULL;
		free(__db_records->array[i].nodename), __db_records->array[i].nodename = NULL;
	}
	free(__db_records);

	return applicative_err;
}


ssize_t commf_read(int fd, char *buf, size_t count){
	struct sock_file_t* sockf = NULL;
	int capab = capabilities_for_file_fd(fd);
	ssize_t read_bytes = -1;
	if ( !__zpool ) return -1;
	if ( EREAD != capab ) return -1;

	sockf = __netw_if->sockf_by_fd(__zpool, fd);
	if ( sockf ){
		if ( sockf->sock_type == ESOCKET_REQREP ){
			struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
			NaClLog(LOG_INFO, "%s() attempt read from fd=%d, count=%d\n", __func__, fd, (int)count );
			/*use socket by REQ-REP pattern. Should be used in next way:
			 * write request HEADER
			 * read  response data*/
			header.req_len = count;
			//request data size we want read
			NaClLog(LOG_INFO, "%s() write fd=%d header size=%d\n", __func__, fd, (int)sizeof(header) );
			assert( sizeof(header) == __netw_if->write_sockf(sockf, (const char*)&header, sizeof(header) ) );
			//read requested data
			read_bytes = __netw_if->read_sockf(sockf, buf, count);
			NaClLog(LOG_INFO, "%s() read ok from fd=%d, requested%d, readed=%d\n", __func__, fd, (int)count, (int)read_bytes );
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
	if ( EWRITE != capab ) return -1;

	sockf = __netw_if->sockf_by_fd(__zpool, fd);
	if ( sockf ){
		if ( sockf->sock_type == ESOCKET_REQREP ){
			size_t sdata = 0;
			/*use socket by REQ-REP pattern. Should be used in next way:
			 * read request HEADER
			 * write requested data*/
			struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
			NaClLog(LOG_INFO, "%s() attempt write to fd=%d, count=%d\n", __func__, fd, (int)count );
			//read request for data
			assert( sizeof(header) == __netw_if->read_sockf(sockf, (char*)&header, sizeof(header)));
			assert(header.protoid == PROTOID);
			//write only requested data from header.len, or all data if user specifies less
			sdata = min(header.req_len, count);
			assert( sdata == __netw_if->write_sockf(sockf, buf, sdata ) );
			wrote_bytes = sdata;
			NaClLog(LOG_INFO, "%s() read ok from fd=%d, requested%d, readed=%d\n", __func__, fd, (int)count, (int)wrote_bytes );
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



