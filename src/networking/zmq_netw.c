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

	/*free array*/
	if ( zpool->sockf_array ){
		for ( int i=0; i < zpool->count_max; i++ ){
			free(zpool->sockf_array[i].tempbuf);
		}
		free(zpool->sockf_array), zpool->sockf_array = NULL;
	}

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


struct sock_file_t* get_dual_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd){
	struct sock_file_t *dual_sockf = NULL;
	struct db_record_t* record1 = NULL;
	struct db_record_t* record2 = NULL;
	struct db_record_t* db_record = match_db_record_by_fd( db_records, fd);

	NaClLog(LOG_INFO, "%p, %p", (void*)zpool, (void*)db_records);
	/*search existing dual direction socket*/
	if ( !db_record ) return NULL; /*no records found, can be illegal using */

	/*Dual direction socket has identical socket params but fmode read / write mode is different*/
	for ( int i=0; i < db_records->count; i++ ){
		if ( !strcmp(db_records->array[i].endpoint, db_record->endpoint ) &&
				db_records->array[i].method == db_record->method &&
				db_records->array[i].sock == db_record->sock ){
			if ( db_records->array[i].fmode == 'r' )
				record1 = &db_records->array[i];
			if ( db_records->array[i].fmode == 'w' )
				record2 = &db_records->array[i];
		}
	}
	/*If requested fd should be use dual direction socket*/
	if ( record1 && record2 ){
		struct sock_file_t *sockf1 = sockf_by_fd(zpool, record1->fd );
		struct sock_file_t *sockf2 = sockf_by_fd(zpool, record2->fd );
		/*if found socket then use exiting zeromq socket*/
		if ( sockf1 )
			dual_sockf = sockf1;
		else if ( sockf2 )
			dual_sockf = sockf2;
	}
	return dual_sockf;
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
	 * Flow1: For non existing, non dual socket add new socket record and next create&init zmq network socket;
	 * Flow2: For file descriptor with existing associated dual direction socket, add new socket record into
	 * array of sockets, but use already existing zmq networking socket for both 'r','w' descriptors;*/
	db_record = match_db_record_by_fd( db_records, fd);
	if ( db_record ){
		/*create new socket record {sock_file_t}*/
		sockf = malloc( sizeof(struct sock_file_t) );
		if ( !sockf ){
			/*no memory allocated
			 * This code section can't be covered by unit tests because it's not depends from input params*/
			NaClLog(LOG_ERROR, "sockf malloc NULL");
		}
		else{
			struct sock_file_t* dual_sockf = NULL;
			memset(sockf, '\0', sizeof(struct sock_file_t));
			sockf->fs_fd = db_record->fd;
			sockf->sock_type = db_record->sock;
			sockf->tempbuf = NULL;

			/*search existing dual direction socket should be only one zeromq socket for two files with
			 * the same endpoint (it's a trick for dual direction sockets)*/
			dual_sockf = get_dual_sockf(zpool, db_records, fd);
			if ( dual_sockf ){
				/*Flow2: dual direction socket, use existing zmq network socket*/
				NaClLog(LOG_INFO, "zvm_open use dual socket");
				sockf->netw_socket = dual_sockf->netw_socket;
			}
			else{
				/*Flow1: init zmq network socket in normal way*/
				NaClLog(LOG_INFO, "open socket: %s, sock type %d\n", db_record->endpoint, db_record->sock);
				sockf->netw_socket = zmq_socket( zpool->context, db_record->sock );
				if ( !sockf->netw_socket ){
					NaClLog(LOG_ERROR, "zmq_socket return NULL");
					free(sockf), sockf=NULL;
				}
				else{
					int err = ERR_OK;
					switch(db_record->method){
					case EMETHOD_BIND:
						NaClLog(LOG_INFO, "open socket: bind");
						err= zmq_bind(sockf->netw_socket, db_record->endpoint);
						NaClLog(LOG_ERROR, "zmq_bind status err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
						break;
					case EMETHOD_CONNECT:
						NaClLog(LOG_INFO, "open socket: connect");
						err = zmq_connect(sockf->netw_socket, db_record->endpoint);
						NaClLog(LOG_INFO, "zmq_connect status err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
						break;
					default:
						NaClLog(LOG_ERROR, "open socket: undefined sock method");
						err = ERR_ERROR;
						break;
					}

					if ( err != ERR_OK ){
						NaClLog(LOG_ERROR, "close opened socket, free sockf, because connect|bind failed");
						zmq_close( sockf->netw_socket );
						free(sockf), sockf = NULL;
					}
				}
			}
			if ( sockf ){
				int err = add_sockf_copy_to_array(zpool, sockf);
				if ( ERR_OK != err ){
					/* This code section can't be covered by unit tests because it's system relative error
					 * and additionaly function check ERR_ALREADY_EXIST case and return it*/
					NaClLog(LOG_ERROR, "add_sockf_copy_to_array %d", err);
					/*if socket opened and used by another sockf, then do not close it*/
					if ( !dual_sockf ){
						free(sockf->netw_socket);
						free(sockf);
					}
					else{
						zmq_close(sockf->netw_socket);
					}
				}
			}
		}
	}
	return sockf;
}


int close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf){
	int err = ERR_OK;
	int zmq_sock_is_used_twice = 0;
	NaClLog(LOG_INFO, "%p, %p", (void*)zpool, (void*)sockf);

	if ( !zpool || !sockf ) return ERR_BAD_ARG;
	NaClLog(LOG_INFO, "fd=%d", sockf->fs_fd);

	for (int i=0; i < zpool->count_max; i++){
		if ( !zpool->sockf_array[i].unused && zpool->sockf_array[i].netw_socket == sockf->netw_socket )
			zmq_sock_is_used_twice++;
	}
	if ( zmq_sock_is_used_twice > 1 ){
		if ( sockf->tempbuf ){
			free(sockf->tempbuf->buf), sockf->tempbuf->buf=NULL;
			free(sockf->tempbuf), sockf->tempbuf = NULL;
		}
		NaClLog(LOG_INFO, "dual direction zmq socket should be closed later");
	}else if( sockf->netw_socket ){
		int err = 0;
		NaClLog(LOG_INFO, "zmq socket closing...");
		err = zmq_close( sockf->netw_socket );
		sockf->netw_socket = NULL;
		NaClLog(LOG_INFO, "zmq_close status err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
	}
	if ( ERR_OK == err )
		return remove_sockf_from_array_by_fd(zpool, sockf->fs_fd );
	else
		return err;
}


ssize_t  write_sockf(struct sock_file_t *sockf, const char *buf, size_t size){
	int err = 0;
	ssize_t wrote = 0;
	zmq_msg_t msg;
	NaClLog(LOG_INFO, "%p, %p, %d", (void*)sockf, (void*)buf, (int)size);
	if ( !sockf || !buf || !size || size==SIZE_MAX ) return 0;

	err = zmq_msg_init_size (&msg, size);
	if ( err != 0 ){
		wrote = 0;
		NaClLog(LOG_ERROR, "zmq_msg_init_size err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
	}
	else{
		memcpy (zmq_msg_data (&msg), buf, size);
		NaClLog(LOG_INFO, "zmq_sending fd=%d buf %d bytes...", sockf->fs_fd, (int)size );
		err = zmq_send ( sockf->netw_socket, &msg, 0);
		if ( err != 0 ){
			NaClLog(LOG_ERROR, "zmq_send err %d, errno %d, status %s\n", err, zmq_errno(), zmq_strerror(zmq_errno()));
			wrote = 0; /*nothing sent*/
		}
		else{
			wrote = size;
			NaClLog(LOG_INFO, "zmq_send ok");
		}
		zmq_msg_close (&msg);
	}
	__bytes_sent +=wrote;
	return wrote;
}


ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count){
	int bytes_read_from_tempbuf = 0;
	int bytes_read_from_socket = 0;
	NaClLog(LOG_INFO, "%p, %p, %d", (void*)sockf, (void*)buf, (int)count);
	if ( !sockf || !buf || !count || count==SIZE_MAX ) return 0;
	/*use sockf->tempbuf to save received results if user received more data than can be write into buf
	 * Data from temp buf should be readed at next call of read*/

	NaClLog(LOG_INFO, "count=%d", (int)count);
	if ( sockf->tempbuf && sockf->tempbuf->buf ){
		NaClLog(LOG_INFO, "read from tempbuf, size=%d, pos=%d", (int)sockf->tempbuf->size, sockf->tempbuf->pos  );
		/*how much of data can be copied, but not more than requested*/
		bytes_read_from_tempbuf = min( sockf->tempbuf->size-sockf->tempbuf->pos, count );
		/*copy to buf param from temp buffer starting from current pos*/
		memcpy (buf, sockf->tempbuf->buf+sockf->tempbuf->pos, bytes_read_from_tempbuf);
		/*update temp buf pos, increase for amount bytes read*/
		sockf->tempbuf->pos += bytes_read_from_tempbuf;
		/*temp buf completely readed*/
		if ( sockf->tempbuf->size <= sockf->tempbuf->pos ){
			free( sockf->tempbuf->buf ), sockf->tempbuf->buf = NULL;
			free( sockf->tempbuf ), sockf->tempbuf = NULL;
			if ( bytes_read_from_tempbuf < count ){
				/*rest of data should be readed from zmq socket*/
				count = count-bytes_read_from_tempbuf;
				buf = buf+bytes_read_from_tempbuf; /*set new offset just after wrote data*/
			}
			else{
				__bytes_recv += bytes_read_from_tempbuf;
				return bytes_read_from_tempbuf;
			}
		}
		else
			__bytes_recv += bytes_read_from_tempbuf;
			return bytes_read_from_tempbuf;
	}

	if ( sockf->netw_socket ){
		do{
			int err = 0;
			void *recv_data = NULL;
			size_t msg_size;
			zmq_msg_t msg;
			zmq_msg_init (&msg);
			NaClLog(LOG_INFO, "zmq_recv fd=%d, %d bytes...", sockf->fs_fd, (int)count);
			err = zmq_recv ( sockf->netw_socket, &msg, 0);
			if ( 0 != err ){
				/*read error*/
				NaClLog(LOG_INFO, "zmq_recv err %d, errno %d, status %s", err, zmq_errno(), zmq_strerror(zmq_errno()) );
				return 0;
			}
			/*read ok*/
			msg_size = zmq_msg_size (&msg);
			NaClLog(LOG_ERROR, "zmq_recv %d bytes ok\n", (int)msg_size);
			bytes_read_from_socket = min( msg_size, count );
			recv_data = zmq_msg_data (&msg);
			memcpy (buf, recv_data, bytes_read_from_socket);
			//if readed data less than received data, then store rest in tempbuf
			if ( bytes_read_from_socket < msg_size ){
				void *data = NULL;
				sockf->tempbuf = malloc( sizeof(struct tempbuf_t) );
				/*initialization of all tempbuf members*/
				sockf->tempbuf->buf = malloc( msg_size-bytes_read_from_socket );
				memset(sockf->tempbuf->buf, '\0', msg_size-bytes_read_from_socket);
				data = (void*)((uintptr_t)recv_data + (uintptr_t)bytes_read_from_socket);
				memcpy( sockf->tempbuf->buf, data, msg_size-bytes_read_from_socket );
				sockf->tempbuf->size = msg_size-bytes_read_from_socket;
				sockf->tempbuf->pos=0;
			}
		}while(bytes_read_from_tempbuf+bytes_read_from_socket < count);
	}
	__bytes_recv += bytes_read_from_tempbuf+bytes_read_from_socket;
	return bytes_read_from_tempbuf+bytes_read_from_socket;
}


int open_all_comm_files(struct zeromq_pool* zpool, struct db_records_t *db_records){
	int err = ERR_OK;
	NaClLog(LOG_INFO, "%p, %p", (void*)zpool, (void*)db_records);
	for (int i=0; i < db_records->count; i++){
		if ( EFILE_MSQ == db_records->array[i].ftype ){
			struct db_record_t *record = &db_records->array[i];
			if ( !open_sockf(zpool, db_records, record->fd) ){
				NaClLog(LOG_ERROR, "fd=%d, error NULL sockf", db_records->array[i].fd);
				return ERR_ERROR;
			}
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


