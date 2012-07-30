/*
 * zmq_netw.c
 *
 *  Created on: 03.05.2012
 *      Author: YaroslavLitvinov
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include <zmq.h>

#include "src/service_runtime/dyn_array.h"
#include "src/service_runtime/nacl_error_code.h"
#include "src/manifest/mount_channel.h" /* d'b */
#include "src/utils/tools.h" /* d'b */
#include "src/platform/nacl_log.h"

#include "src/networking/sqluse_srv.h"
#include "src/networking/zmq_netw.h"
#include "src/networking/sqluse_srv.h"

static uint32_t __bytes_recv = 0;
static uint32_t __bytes_sent = 0;
/*pluggable facade*/
static struct zeromq_interface *__io_if = NULL;

/*Do not allow zeromq objective code just here for unit tests*/
#ifndef UNIT_TEST

/*Initialization I/O interface by zeromq functions. Should be used only for interface initialization*/
static struct zeromq_interface __zeromq_implementation =
{
  .init = zmq_init,
  .term = zmq_term,
  .send = (int(*)(void *, void *, int)) zmq_send,
  .recv = (int(*)(void *, void *, int)) zmq_recv,
  .bind = zmq_bind,
  .connect = zmq_connect,
  .open_socket = zmq_socket,
  .close_socket = zmq_close,
  .msg_init_size = (int(*)(void *, size_t)) zmq_msg_init_size,
  .msg_init = (int(*)(void *)) zmq_msg_init,
  .msg_data = (void*(*)(void *)) zmq_msg_data,
  .msg_size = (size_t(*)(void *)) zmq_msg_size,
  .msg_close = (int(*)(void *)) zmq_msg_close,
  .errno_io = zmq_errno,
  .strerror = zmq_strerror,
  .realloc = realloc,
  .malloc = malloc,
  .calloc = calloc
};

struct zeromq_interface* zeromq_interface_implementation()
{
  return &__zeromq_implementation;
}
#endif

int init_zeromq_pool(struct zeromq_interface *io_interface, struct zeromq_pool * zpool)
{
  int err = LOAD_OK;
  assert( io_interface != NULL);
  assert( zpool != NULL);
  __io_if = io_interface;

  /*create context, it should be destroyed in zeromq_term; all opened sockets should be closed
   *before finishing, either zmq_term will wait in internal loop for completion of all I/O requests*/
  zpool->context = __io_if->init(ZMQ_THREAD_COUNT);
  if(zpool->context != NULL)
  {
    /*allocated memory for array should be free at the zeromq_term */
    zpool->sockf_array = __io_if->calloc(1, sizeof(struct DynArray));
    assert( zpool->sockf_array != NULL);
    assert( 1 == DynArrayCtor(zpool->sockf_array, ESOCKF_ARRAY_GRANULARITY));
  }
  else
  {
    NaClLog(LOG_ERROR, "__io_if->init errno %d errtext %s\n", __io_if->errno_io(),
            __io_if->strerror(__io_if->errno_io()));
    err = LOAD_0MQ_CONTEXT_INIT_FAILED;
  }
  return err;
}

int zeromq_term(struct zeromq_pool* zpool)
{
  int err = LOAD_OK;
  assert( zpool != NULL);
  assert( zpool->context != NULL);
  assert( zpool->sockf_array != NULL);
  for(int i = 0; i < zpool->sockf_array->num_entries; i++)
  {
    free(zpool->sockf_array->ptr_array[i]);
  }
  DynArrayDtor(zpool->sockf_array);
  free(zpool->sockf_array), zpool->sockf_array = NULL;

  /*destroy zmq context*/
  err = __io_if->term(zpool->context);
  if(0 != err)
  {
    err = LOAD_0MQ_CONTEXT_TERM_FAILED;
    NaClLog(LOG_ERROR, "__io_if->term errno %d errtext %s\n", __io_if->errno_io(),
            __io_if->strerror(__io_if->errno_io()));
  }
  return err;
}

struct sock_file_t* sockf_by_fd(struct zeromq_pool* zpool, int fd)
{
  if(zpool && zpool->sockf_array)
  {
    struct sock_file_t *sockf = NULL;
    for(int i = 0; i < zpool->sockf_array->num_entries; i++)
    {
      sockf = (struct sock_file_t*) (zpool->sockf_array->ptr_array[i]);
      if(sockf->fs_fd == fd) return sockf;
    }
  }
  return NULL;
}

int add_sockf_copy_to_array(struct zeromq_pool* zpool, struct sock_file_t* sockf)
{
  struct sock_file_t* sockf_add = NULL;
  int err = LOAD_OK;
  if(sockf_by_fd(zpool, sockf->fs_fd) != NULL)
  {
    return LOAD_ITEM_ALREADY_EXIST;
  }
  sockf_add = __io_if->malloc(sizeof(struct sock_file_t));
  assert( sockf_add != NULL);
  *sockf_add = *sockf;
  /*if save to array failed*/
  assert( DynArraySet( zpool->sockf_array, zpool->sockf_array->num_entries, sockf_add ) != 0);
  return err;
}

struct sock_file_t* open_sockf(struct zeromq_pool* zpool, struct db_records_t *db_records,
    int fd)
{
  struct sock_file_t *sockf = NULL;
  struct db_record_t* db_record = NULL;
  sockf = sockf_by_fd(zpool, fd);
  if(sockf != NULL)
  {
    /*file with predefined descriptor already opened, just return socket*/
    NaClLog(LOG_INFO, "Existing socket: Trying to open twice? fd=%d", sockf->fs_fd);
    return sockf;
  }

  /* Trying to open new msq file because socket asociated with file descriptor is not found,
   * trying to open socket in normal way, first search socket data associated with file descriptor
   * in channels DB; From found db_record retrieve socket details data and start sockf opening flow;
   * Flow: For non existing socket add new socket record and next create&init zmq network socket;*/
  for(int i = 0; i < db_records->count; i++)
  {
    if(db_records->array[i].fd == fd) db_record = &db_records->array[i];
  }
  if(db_record)
  {
    /*create new socket record {sock_file_t}*/
    sockf = __io_if->calloc(1, sizeof(struct sock_file_t));
    if(NULL == sockf)
    {
      /*no memory allocated*/
      NaClLog(LOG_ERROR, "sockf malloc NULL\n");
    }
    else
    {
      int err = 0;
      sockf->fs_fd = db_record->fd;
      sockf->sock_type = db_record->sock;
      sockf->access_mode = db_record->fmode;

      /*init zmq network socket: open, bind|connect*/
      switch(db_record->sock)
      {
        case ESOCKET_REQREP:
          NaClLog(LOG_INFO, "open socket: ESOCKET_REQREP\n");
          sockf->capabilities = EREADWRITE;
          if('r' == db_record->fmode)
          {
            NaClLog(LOG_INFO, "open socket: %s, sock type ZMQ_REQ\n", db_record->endpoint);
            sockf->netw_socket = __io_if->open_socket(zpool->context, ZMQ_REQ);
            if(sockf->netw_socket)
            {
              if(__io_if->connect(sockf->netw_socket, db_record->endpoint) != 0)
              {
                err = -1;
                NaClLog(LOG_ERROR, "Error __io_if->connect errno %d, status %s\n",
                        __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
              }
              else
              {
                NaClLog(LOG_INFO, "__io_if->connect OK \n");
              }
            }
            else
            {
              err = -1;
              NaClLog(LOG_ERROR, "Error __io_if->open_socket errno %d, status %s\n",
                      __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
            }
          }
          else if('w' == db_record->fmode)
          {
            NaClLog(LOG_INFO, "open socket: %s, sock type ZMQ_REP\n", db_record->endpoint);
            sockf->netw_socket = __io_if->open_socket(zpool->context, ZMQ_REP);
            if(sockf->netw_socket)
            {
              if(__io_if->bind(sockf->netw_socket, db_record->endpoint) != 0)
              {
                err = -1;
                NaClLog(LOG_ERROR, "Error __io_if->bind errno %d, status %s\n",
                        __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
              }
              else
              {
                NaClLog(LOG_INFO, "__io_if->bind OK \n");
              }
            }
            else
            {
              err = -1;
              NaClLog(LOG_ERROR, "Error __io_if->open_socket errno %d, status %s\n",
                      __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
            }
          }
          else
          {
            NaClLog(LOG_ERROR, "Can't open socket: Unknown mode %c\n", db_record->fmode);
            err = -1;
          }
          break;
        case ESOCKET_UNKNOWN:
        default:
          NaClLog(LOG_ERROR, "open socket: unknown socket\n");
          err = -1;
          break;
      }

      if(err != 0)
      {
        if(sockf->netw_socket)
        {
          NaClLog(LOG_ERROR,
                  "Open socket error : close opened socket, because connect|bind failed\n");
          __io_if->close_socket(sockf->netw_socket);
        }
        NaClLog(LOG_ERROR, "Open socket error : free sockf\n");
        free(sockf), sockf = NULL;
      }

      if(sockf)
      {
        add_sockf_copy_to_array(zpool, sockf);
      }
    }
  }
  return sockf;
}

int close_sockf(struct zeromq_pool* zpool, struct sock_file_t *sockf)
{
  int saved_err = 0;
  int err = 0;
  NaClLog(LOG_INFO, "close_sockf fd=%d\n", sockf->fs_fd);

  if(sockf->netw_socket)
  {
    NaClLog(LOG_INFO, "zmq socket closing...\n");
    saved_err = __io_if->close_socket(sockf->netw_socket);
    sockf->netw_socket = NULL;
    /*erase closed sockf, memory will be freed in array destroy*/
    memset(sockf, '\0', sizeof(struct sock_file_t));
    NaClLog(LOG_INFO, "__io_if->close_socket errcode=%d\n", err);
  }
  if(LOAD_OK != saved_err) return saved_err;
  else return err;
}

ssize_t write_sockf(struct sock_file_t *sockf, const char *buf, size_t size)
{
  int err = 0;
  ssize_t wrote = -1;
  zmq_msg_t msg;
  NaClLog(LOG_INFO, "%p, %p, %d\n", (void*) sockf, (void*) buf, (int) size);
  if(!sockf || !buf || !size || size == SIZE_MAX) return -1;
  if(EWRITE != sockf->capabilities && EREADWRITE != sockf->capabilities) return -1;

  err = __io_if->msg_init_size(&msg, size);
  if(err != 0)
  {
    NaClLog(LOG_ERROR, "__io_if->msg_init_size err %d, errno %d, status %s\n", err,
            __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
  }
  else
  {
    memcpy(__io_if->msg_data(&msg), buf, size);
    NaClLog(LOG_INFO, "__io_if->sending fd=%d buf %d bytes via socket %p...\n", sockf->fs_fd,
            (int) size, sockf->netw_socket);
    err = __io_if->send(sockf->netw_socket, &msg, 0);
    if(err != 0)
    {
      NaClLog(LOG_ERROR, "__io_if->send err %d, errno %d, status %s\n", err,
              __io_if->errno_io(), __io_if->strerror(__io_if->errno_io()));
    }
    else
    {
      wrote = size;
      NaClLog(LOG_INFO, "__io_if->send ok\n");
    }
    __io_if->msg_close(&msg);
  }
  if(wrote > 0) __bytes_sent += wrote;
  return wrote;
}

ssize_t read_sockf(struct sock_file_t *sockf, char *buf, size_t count)
{
  int bytes_read_from_socket = 0;
  NaClLog(LOG_INFO, "%p, %p, %d\n", (void*) sockf, (void*) buf, (int) count);
  if(!sockf || !buf || !count || count == SIZE_MAX) return -1;
  if(EREAD != sockf->capabilities && EREADWRITE != sockf->capabilities) return -1;

  NaClLog(LOG_INFO, "count=%d", (int) count);
  if(sockf->netw_socket)
  {
    int err = 0;
    void *recv_data = NULL;
    size_t msg_size;
    zmq_msg_t msg;
    __io_if->msg_init(&msg);
    NaClLog(LOG_INFO, "__io_if->recv fd=%d, %d bytes via socket=%p...\n", sockf->fs_fd,
            (int) count, sockf->netw_socket);
    err = __io_if->recv(sockf->netw_socket, &msg, 0);
    if(0 != err)
    {
      /*read error*/
      NaClLog(LOG_INFO, "__io_if->recv err %d, errno %d, status %s\n", err, __io_if->errno_io(),
              __io_if->strerror(__io_if->errno_io()));
      __io_if->msg_close(&msg);
      return -1;
    }
    /*read ok*/
    /*copy recv bytes into buf result*/
    msg_size = __io_if->msg_size(&msg);
    NaClLog(LOG_ERROR, "__io_if->recv %d bytes ok\n", (int) msg_size);
    bytes_read_from_socket = min( msg_size, count );
    recv_data = __io_if->msg_data(&msg);
    memcpy(buf, recv_data, bytes_read_from_socket);
    __io_if->msg_close(&msg);
    if(bytes_read_from_socket > 0) __bytes_recv += bytes_read_from_socket;
  }
  return bytes_read_from_socket;
}

int open_all_comm_files(struct zeromq_pool* zpool, struct db_records_t *db_records)
{
  int err = LOAD_OK;
  NaClLog(LOG_INFO, "open_all_comm_files %p, %p", (void*) zpool, (void*) db_records);
  for(int i = 0; i < db_records->count; i++)
  {
    struct db_record_t *record = &db_records->array[i];
    if(NULL == open_sockf(zpool, db_records, record->fd))
    {
      NaClLog(LOG_ERROR, "fd=%d, error NULL sockf", db_records->array[i].fd);
      return LOAD_0MQ_SOCKET_ERROR;
    }
  }
  return err;
}

int close_all_comm_files(struct zeromq_pool* zpool)
{
  int err = LOAD_OK;
  struct sock_file_t *sockf = NULL;
  NaClLog(LOG_INFO, "close_all_comm_files %p", (void*) zpool);
  for(int i = 0; i < zpool->sockf_array->num_entries; i++)
  {
    sockf = zpool->sockf_array->ptr_array[i];
    if(close_sockf(zpool, sockf) != LOAD_OK)
    {
      err = LOAD_0MQ_SOCKET_ERROR;
    }
    NaClLog(LOG_ERROR, "close_all_comm_files : close_sockf fd=%d, error\n", sockf->fs_fd);
  }
  return err;
}

