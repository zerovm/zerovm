/*
 * zmq_netw.h
 *
 *  Created on: 03.05.2012
 *      Author: YaroslavLitvinov
 * Server side code.
 * This communication engine is implementation of channel communications;
 * It's code can be used unchanged for interprocess and tcp communications because networking configuration
 * is completely described by db_records data;
 * It's used by nodes as layer between file I/O and networking where write_sockf, read_sockf
 * main input output routines. Every communication socket (sock_file_t) is related to own fd - file descriptor;
 * It uses Zero MQ messaging library, and currently support only REQ-REP pattern:
 * REQ-REP use dual direction zeromq sockets; REQ writes/reads data; REP reads/writes data;
 * Pair of sock_file_t file sockets related to single REQ/REP zmq socket should be used
 * sequently: one read, one write otherwise ZeroMQ will raise errors;
 */

#ifndef NETWORKING_ZMQ_NETW_H_
#define NETWORKING_ZMQ_NETW_H_

#include <sys/types.h>
#include <stddef.h>
#include "sqluse_srv.h"

//forward declaration
struct DynArray;

#define min(a,b) (a < b ? a : b )

enum SockCapability{ ENOTALLOWED=0, EREAD=1, EWRITE=2, EREADWRITE=3};

/*Increase zmq internal thread count if need to get more than 1Gb/s throughput*/
#define ZMQ_THREAD_COUNT 1

struct zeromq_interface {
	void* (*init) (int io_threads);
	int (*term) (void *context);
	int (*send) (void *socket, void *zmq_msg, int flags);
	int (*recv) (void *s, void *zmq_msg, int flags);
	int (*bind) (void *s, const char *addr);
	int (*connect) (void *s, const char *addr);
	void* (*open_socket) (void *context, int type);
	int (*close_socket) (void *s);
	int (*msg_init_size) (void *zmq_msg, size_t size);
	int (*msg_init) (void *zmq_msg);
	void* (*msg_data) (void *zmq_msg);
	size_t (*msg_size) (void *zmq_msg);
	int (*msg_close) (void *zmq_msg);
	int (*errno_io) (void);
	const char *(*strerror) (int errnum);
	void *(*realloc)(void *ptr, size_t size);
	void *(*malloc)(size_t size);
	void *(*calloc)(size_t nmemb, size_t size);
};

void GetZeroMqNativeInterface( struct zeromq_interface* );

enum {ESOCKET_UNKNOWN, ESOCKET_REQREP=1};

struct sock_file_t{
	void *netw_socket;
	int sock_type;
	char access_mode; //'w', 'r'
	int capabilities;
	int fs_fd;
};

enum { ESOCKF_ARRAY_GRANULARITY=10 };
struct zeromq_pool{
	void *context;
	struct DynArray* sockf_array;
};


/*find sock in array*/
struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd);
/*add sock to array*/
int add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf);

/*get interface initialized by zeromq impementation*/
struct zeromq_interface* zeromq_interface_implementation();
/* zeromq_io Caller should not destroy interface implementation till the terminate;
 * init zmq, init array of sockets
 * @return NaClErrorCode*/
int init_zeromq_pool(struct zeromq_interface* zeromq_io, struct zeromq_pool * zpool);
/*free zmq resources, and sockets array
 * @return NaClErrorCode::LOAD_0MQ_CONTEXT_TERM_FAILED if context termination failed*/
int zeromq_term(struct zeromq_pool* zpool);

/*open file socket is associated with fd; It's should be called close_sockf for every opened file socket;
 * @param zpool should be valid and preconfigured by init_zeromq_pool
 * @param db_records should contains complete db
 * @return opened file socket object, owned by zpool; return NULL if no fd found in db_records*/
struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records, int fd);

/*close file socket, opened by open_sockf; it's do not removing it from sockets array only free zeromq resources;
 * @return err*/
int close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf);

/*stream write file socket*/
ssize_t write_sockf(struct sock_file_t *sockf, const char *buf, size_t count);
/*stream read file socket*/
ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count);

/*open all sockets connections; It can be used instead explicitly opening of file sockets;
 *Use close_all_comm_files to close file sockets;  */
int open_all_comm_files(struct zeromq_pool* zpool, struct db_records_t *db_records);
/*Close all connections; used in pair with open_all_comm_files*/
int close_all_comm_files(struct zeromq_pool* zpool);

#endif /* NETWORKING_ZMQ_NETW_H_ */
