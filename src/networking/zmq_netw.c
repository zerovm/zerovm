/*
 * zmq_netw.c
 *
 *  Created on: 03.05.2012
 *      Author: YaroslavLitvinov
 */


#include "src/networking/sqluse_srv.h"
#include "src/networking/zmq_netw.h"
#include "src/networking/sqluse_srv.h"
#include "src/networking/errcodes.h"
#include "src/platform/nacl_log.h"
#include "src/networking/errcodes.h"
#include <zmq.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static uint32_t __bytes_recv = 0;
static uint32_t __bytes_sent = 0;

int init_zeromq_pool(struct zeromq_pool * zpool){
	int err = 0;
	NaClLog(LOG_INFO, "%p", (void*)zpool);
	if ( !zpool ) return ERR_BAD_ARG;

	/*create zmq context, it should be destroyed in zeromq_term; all opened sockets should be closed
	 *before finishing, either zmq_term will wait in internal loop for completion of all I/O requests*/
	zpool->context = zmq_init(1);
	if ( zpool->context ){
		zpool->count_max=ESOCKF_ARRAY_GRANULARITY;
		/*allocated memory for array should be free at the zeromq_term */
		zpool->sockf_array = malloc(zpool->count_max * sizeof(struct sock_file_t));
		if ( zpool->sockf_array ) {
			memset(zpool->sockf_array, '\0', zpool->count_max*sizeof(struct sock_file_t));
			for (int i=0; i < zpool->count_max; i++)
				zpool->sockf_array[i].unused = 1;
			err = ERR_OK;
		}
		else{
			/*no memory allocated
			 * This code section can't be covered by unit tests because it's not depends from zpool parameter;
			 * sockf_array member is completely setups internally*/
			NaClLog(LOG_ERROR, "%d bytes alloc error", (int)sizeof(struct sock_file_t));
			err = ERR_NO_MEMORY;
		}
	}
	else{
		/* This code section can't be covered by unit tests because it's not depends from zpool parameter;
		 * it's only can be produced as unexpected zeromq error*/
		NaClLog(LOG_ERROR, "zmq_init err %d, errno %d errtext %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
		err = ERR_ERROR;
	}
	return err;
}

int zeromq_term(struct zeromq_pool* zpool){
	int err = ERR_OK;
	NaClLog(LOG_INFO, "%p", (void*)zpool);
	if ( !zpool ) return ERR_BAD_ARG;

	free(zpool->sockf_array), zpool->sockf_array = NULL;

	/*destroy zmq context*/
	if (zpool->context){
		int err= zmq_term(zpool->context);
		if ( err != 0 ){
			err = ERR_ERROR;
			NaClLog(LOG_ERROR, "zmq_term err %d, errno %d errtext %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
		}
		else{
			err= ERR_OK;
			zpool->context = NULL;
			NaClLog(LOG_ERROR, "zmq_term %d\n", 1);
		}
	}
	else{
		NaClLog(LOG_ERROR, "context NULL %d", 0);
		err = ERR_ERROR;
	}
	return err;
}

struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd){
	if ( zpool && zpool->sockf_array ){
		for (int i=0; i < zpool->count_max; i++)
			if ( !zpool->sockf_array[i].unused && zpool->sockf_array[i].fs_fd == fd )
				return &zpool->sockf_array[i];
	}
	return NULL;
}

int add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf){
	struct sock_file_t* sockf_add = NULL;
	int err = ERR_OK;
	NaClLog(LOG_INFO, "%p, %p", (void*)zpool, (void*)sockf);

	if ( !zpool || !sockf ) return ERR_BAD_ARG;
	if ( !zpool->sockf_array) return ERR_BAD_ARG;
	if( sockf_by_fd(zpool, sockf->fs_fd) ) return ERR_ALREADY_EXIST;

	do{
		/*search unused array cell*/
		for (int i=0; i < zpool->count_max; i++)
			if ( zpool->sockf_array[i].unused ){
				sockf_add = &zpool->sockf_array[i];
				break;
			}
		/*extend array if not found unused cell*/
		if ( !sockf_add ){
			zpool->count_max+=ESOCKF_ARRAY_GRANULARITY;
			zpool->sockf_array = realloc(zpool->sockf_array, zpool->count_max * sizeof(struct sock_file_t));
			if ( zpool->sockf_array ){
				for (int i=zpool->count_max-ESOCKF_ARRAY_GRANULARITY; i < zpool->count_max; i++)
					zpool->sockf_array[i].unused = 1;
			}
			else{
				/*no memory re allocated
				 * This code section can't be covered by unit tests because it's system error*/
				err = ERR_ERROR;
				NaClLog(LOG_ERROR, "sockf_array realloc mem failed %d", err);
			}
		}
	}while(!sockf_add || ERR_OK!=err);
	*sockf_add = *sockf;
	sockf_add->unused = 0;
	return err;
}


int remove_sockf_from_array_by_fd(struct zeromq_pool* zpool, int fd){
	int err = ERR_NOT_FOUND;
	NaClLog(LOG_INFO, "%p", (void*)zpool);
	if ( !zpool ) return ERR_BAD_ARG;
	if ( !zpool->sockf_array ) return ERR_BAD_ARG;

	for (int i=0; i < zpool->count_max; i++)
		if ( zpool->sockf_array[i].fs_fd == fd){
			zpool->sockf_array[i].unused = 1;
			err = ERR_OK;
			break;
		}
	return err;
}


struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd){
	struct sock_file_t *sockf = NULL;
	struct db_record_t* db_record = NULL;
	NaClLog(LOG_INFO, "%p, %p, %d", (void*)zpool, (void*)db_records, fd);
	if (!zpool || !db_records) return NULL;

	sockf = sockf_by_fd(zpool, fd );
	if ( sockf ) {
		/*file with predefined descriptor already opened, just return socket*/
		NaClLog(LOG_INFO, "Existing socket: Trying to open twice? %d", sockf->fs_fd);
		return sockf;
	}

	/* Trying to open new msq file because socket asociated with file descriptor is not found,
	 * trying to open socket in normal way, first search socket data associated with file descriptor
	 * in channels DB; From found db_record retrieve socket details data and start sockf opening flow;
	 * Flow: For non existing socket add new socket record and next create&init zmq network socket;*/
	db_record = match_db_record_by_fd( db_records, fd);
	if ( db_record ){
		/*create new socket record {sock_file_t}*/
		sockf = malloc( sizeof(struct sock_file_t) );
		if ( !sockf ){
			/*no memory allocated
			 * This code section can't be covered by unit tests because it's not depends from input params*/
			NaClLog(LOG_ERROR, "sockf malloc NULL\n\n");
		}
		else{
			int err = ERR_OK;
			memset(sockf, '\0', sizeof(struct sock_file_t));
			sockf->fs_fd = db_record->fd;
			sockf->sock_type = db_record->sock;
			sockf->access_mode = db_record->fmode;

			/*init zmq network socket: open, bind|connect*/
			switch(db_record->sock){
			case ESOCKET_REQREP:
				NaClLog(LOG_INFO, "open socket: ESOCKET_REQREP\n");
				sockf->capabilities = EREADWRITE;
				if ( 'r' == db_record->fmode ){
					NaClLog(LOG_INFO, "open socket: %s, sock type ZMQ_REQ\n", db_record->endpoint);
					sockf->netw_socket = zmq_socket( zpool->context, ZMQ_REQ );
					if (sockf->netw_socket){
						err= zmq_connect(sockf->netw_socket, db_record->endpoint);
						NaClLog(LOG_ERROR, "zmq_connect status err %d\n", err);
					}
				}
				else if ( 'w' == db_record->fmode ){
					NaClLog(LOG_INFO, "open socket: %s, sock type ZMQ_REP\n", db_record->endpoint);
					sockf->netw_socket = zmq_socket( zpool->context, ZMQ_REP );
					if (sockf->netw_socket){
						err= zmq_bind(sockf->netw_socket, db_record->endpoint);
						NaClLog(LOG_ERROR, "zmq_bind status err %d\n", err);
					}
				}
				if ( err ){
					NaClLog(LOG_ERROR, "zmq_bind errno %d, status %s\n", zmq_errno(), zmq_strerror(zmq_errno()));
				}
				break;
			case ESOCKET_UNKNOWN:
			default:
				NaClLog(LOG_ERROR, "open socket: unknown socket\n");
				err = ERR_ERROR;
				break;
			}

			if ( err != ERR_OK ){
				NaClLog(LOG_ERROR, "close opened socket, free sockf, because connect|bind failed\n");
				zmq_close( sockf->netw_socket );
				free(sockf), sockf = NULL;
			}

			if ( sockf ){
				int err = add_sockf_copy_to_array(zpool, sockf);
				if ( ERR_OK != err ){
					/* This code section can't be covered by unit tests because it's system relative error
					 * and additionaly function check ERR_ALREADY_EXIST case and return it*/
					NaClLog(LOG_ERROR, "add_sockf_copy_to_array %d\n", err);
					zmq_close(sockf->netw_socket);
					free(sockf);
				}
			}
		}
	}
	return sockf;
}


int close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf){
	int err = ERR_OK;
	NaClLog(LOG_INFO, "%p, %p\n", (void*)zpool, (void*)sockf);

	if ( !zpool || !sockf ) return ERR_BAD_ARG;
	NaClLog(LOG_INFO, "fd=%d\n", sockf->fs_fd);

	if( sockf->netw_socket ){
		int err = 0;
		if ( 'w' == sockf->access_mode ){
			NaClLog(LOG_INFO, "zmq_recv ZMQ_NOBLOCK workaround zmq_send bug\n");
			zmq_msg_t msg;
			zmq_msg_init (&msg);
			err = zmq_recv ( sockf->netw_socket, &msg, ZMQ_NOBLOCK);
			NaClLog(LOG_INFO, "zmq_recv inside close err =%d, status errno %d, status %s\n",
					err, zmq_errno(), zmq_strerror(zmq_errno()));
			zmq_msg_close(&msg);
		}

		NaClLog(LOG_INFO, "zmq socket closing...\n");
		err = zmq_close( sockf->netw_socket );
		sockf->netw_socket = NULL;
		NaClLog(LOG_INFO, "zmq_close status err %d\n", err);
		if (err){
			NaClLog(LOG_INFO, "zmq_close status errno %d, status %s\n", zmq_errno(), zmq_strerror(zmq_errno()));
		}
	}
	if ( ERR_OK == err )
		return remove_sockf_from_array_by_fd(zpool, sockf->fs_fd );
	else
		return err;
}


ssize_t  write_sockf(struct sock_file_t *sockf, const char *buf, size_t size){
	int err = 0;
	ssize_t wrote = -1;
	zmq_msg_t msg;
	NaClLog(LOG_INFO, "%p, %p, %d\n", (void*)sockf, (void*)buf, (int)size);
	if ( !sockf || !buf || !size || size==SIZE_MAX ) return -1;
	if ( EWRITE != sockf->capabilities && EREADWRITE != sockf->capabilities  ) return -1;

	err = zmq_msg_init_size (&msg, size);
	if ( err != 0 ){
		NaClLog(LOG_ERROR, "zmq_msg_init_size err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
	}
	else{
		memcpy (zmq_msg_data (&msg), buf, size);
		NaClLog(LOG_INFO, "zmq_sending fd=%d buf %d bytes via socket %p...\n", sockf->fs_fd, (int)size, sockf->netw_socket );
		err = zmq_send ( sockf->netw_socket, &msg, 0);
		if ( err != 0 ){
			NaClLog(LOG_ERROR, "zmq_send err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
		}
		else{
			wrote = size;
			NaClLog(LOG_INFO, "zmq_send ok\n");
		}
		zmq_msg_close (&msg);
	}
	if ( wrote > 0 )
		__bytes_sent +=wrote;
	return wrote;
}


ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count){
	int bytes_read_from_socket = 0;
	NaClLog(LOG_INFO, "%p, %p, %d\n", (void*)sockf, (void*)buf, (int)count);
	if ( !sockf || !buf || !count || count==SIZE_MAX ) return -1;
	if ( EREAD != sockf->capabilities && EREADWRITE != sockf->capabilities  ) return -1;

	NaClLog(LOG_INFO, "count=%d", (int)count);
	if ( sockf->netw_socket ){
		int err = 0;
		void *recv_data = NULL;
		size_t msg_size;
		zmq_msg_t msg;
		zmq_msg_init (&msg);
		NaClLog(LOG_INFO, "zmq_recv fd=%d, %d bytes via socket=%p...\n", sockf->fs_fd, (int)count, sockf->netw_socket);
		err = zmq_recv ( sockf->netw_socket, &msg, 0);
		if ( 0 != err ){
			/*read error*/
			NaClLog(LOG_INFO, "zmq_recv err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()) );
			return 0;
		}
		/*read ok*/
		/*copy recv bytes into buf result*/
		msg_size = zmq_msg_size (&msg);
		NaClLog(LOG_ERROR, "zmq_recv %d bytes ok\n", (int)msg_size);
		bytes_read_from_socket = min( msg_size, count );
		recv_data = zmq_msg_data (&msg);
		memcpy (buf, recv_data, bytes_read_from_socket);
		zmq_msg_close (&msg);
		if ( bytes_read_from_socket > 0 )
			__bytes_recv+=bytes_read_from_socket;
	}
	return bytes_read_from_socket;
}


int open_all_comm_files(struct zeromq_pool* zpool, struct db_records_t *db_records){
	int err = ERR_OK;
	NaClLog(LOG_INFO, "open_all_comm_files %p, %p", (void*)zpool, (void*)db_records);
	for (int i=0; i < db_records->count; i++){
		struct db_record_t *record = &db_records->array[i];
		if ( !open_sockf(zpool, db_records, record->fd) ){
			NaClLog(LOG_ERROR, "fd=%d, error NULL sockf", db_records->array[i].fd);
			return ERR_ERROR;
		}
	}
	return err;
}


int close_all_comm_files(struct zeromq_pool* zpool){
	int err = ERR_OK;
	NaClLog(LOG_INFO, "%p", (void*)zpool);
	for (int i=0; i < zpool->count_max; i++){
		if ( !zpool->sockf_array[i].unused )
		err = close_sockf(zpool, sockf_by_fd(zpool, zpool->sockf_array[i].fs_fd));
		if ( ERR_OK != err ){
			NaClLog(LOG_ERROR, "fd=%d, error=%d", zpool->sockf_array[i].fs_fd, err);
			return err;
		}
	}
	return err;
}


