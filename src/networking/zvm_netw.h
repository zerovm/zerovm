/*
 * zvm_netw.h
 *
 *  Created on: 03.06.2012
 *      Author: YaroslavLitvinov
 */

#ifndef NETWORKING_ZVM_NETW_H_
#define NETWORKING_ZVM_NETW_H_

#include <stdint.h>

#define PROTOID 0xABBA
#define PROTOVER 0x01
#define COMMAND_DATA 0x0
#define COMMAND_FIN 0xFF /*used at the socket closing*/
#define DEFAULT_ZVM_NETW_HEAD {PROTOID, PROTOVER, COMMAND_DATA, 0, 0,0,0}

//forward declar
struct db_record_t;
struct db_records_t;
struct zeromq_interface;
struct sock_file_t;

struct zmq_netw_interface
{
  int (*get_all_dbrecords)
      (const char *path, const char *nodename, struct db_records_t *db_records);

  struct db_record_t* (*match_db_record_by_fd)
      (struct db_records_t *records, int fd);

  int64_t (*GetFileSize)(const char *name); /* d'b: tiny fix */

  struct sock_file_t* (*sockf_by_fd)
      (struct zeromq_pool* zpool, int fd);

  int (*init_zeromq_pool)
      (struct zeromq_interface* io_interface, struct zeromq_pool * zpool);

  int (*open_all_comm_files)
      (struct zeromq_pool* zpool, struct db_records_t *db_records);

  int (*close_all_comm_files)(struct zeromq_pool* zpool);

  int (*zeromq_term)(struct zeromq_pool* zpool);

  ssize_t (*write_sockf)(struct sock_file_t *sockf, const char *buf, size_t size);

  ssize_t (*read_sockf)(struct sock_file_t *sockf, char *buf, size_t count);
};

struct zeromq_pool *pool_for_unit_test();
struct db_records_t *db_records_for_unit_test();
struct zmq_netw_interface *zmq_netw_interface_implementation();

struct zvm_netw_header_t
{
  uint16_t protoid;
  uint8_t protover;
  uint8_t command;
  uint32_t req_len;
  uint64_t reserved1;
  uint64_t reserved2;
  uint64_t reserved3;
};

/* dbname db path name
 * netw_if plaggable facade for networking & db; Caller should not destroy interface implementation till the terminate;
 * nodename param for sql query
 * nodeid param for postprocessing endpoints
 * return errcode EZVM..*/
int init_zvm_networking(struct zmq_netw_interface *netw_if,
    const char *dbname, const char *nodename, int nodeid);

int term_zvm_networking();

/*Get capability for file, for unsupported return ENOTALLOWED*/
int capabilities_for_file_fd(int fd);

/*stream write communication file*/
ssize_t commf_write(int fd, const char *buf, size_t count);

/*stream read communication file*/
ssize_t commf_read(int fd, char *buf, size_t count);

#endif /* NETWORKING_ZVM_NETW_H_ */
