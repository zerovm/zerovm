/*
 * Tests for zeromq networking
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>
#include <zmq.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "src/platform/nacl_log.h"
#include "src/service_runtime/nacl_error_code.h"

extern "C" {
#include "src/networking/zmq_netw.h"
#include "src/networking/sqluse_srv.h"
}

#define ENABLE 1
#define DISABLE 0

const char *__empty_str = "\0";
const char *__endpoint1 = "ipc:///tmp/test1\0";

using ::testing::InSequence;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::ReturnNull;
using ::testing::AtLeast;
using ::testing::_;

class ZeromqAbstract {
public:
	virtual ~ZeromqAbstract(){}
	virtual int* Init(int io_threads) = 0;
	virtual int Term(void *context) = 0;
	virtual int Send(void *socket, void *zmq_msg, int flags) = 0;
	virtual int Recv(void *s, void *zmq_msg, int flags) = 0;
	virtual int Bind(void *s, const char *addr) = 0;
	virtual int Connect(void *s, const char *addr) = 0;
	virtual int* OpenSocket(void *context, int type) = 0;
	virtual int CloseSocket(void *socket) = 0;
	virtual int MsgInitSize(void *zmq_msg, size_t size) = 0;
	virtual int MsgInit(void *zmq_msg) = 0;
	virtual int* MsgData(void *zmq_msg) = 0;
	virtual size_t MsgSize(void *zmq_msg) const = 0;
	virtual int MsgClose(void *zmq_msg) = 0;
	virtual int ErrNo() const = 0;
	virtual int* Realloc(void *ptr, size_t size) = 0;
	virtual int* Malloc(size_t size) = 0;
	virtual int* Calloc(size_t nmemb, size_t size) = 0;
};


class MockZeromq : public ZeromqAbstract {
public:
	MOCK_METHOD1(Init, int*(int io_threads) );
	MOCK_METHOD1(Term, int(void *context) );
	MOCK_METHOD3(Send, int(void *socket, void *zmq_msg, int flags) );
	MOCK_METHOD3(Recv, int(void *s, void *zmq_msg, int flags) );
	MOCK_METHOD2(Bind, int(void *s, const char *addr) );
	MOCK_METHOD2(Connect, int(void *s, const char *addr) );
	MOCK_METHOD2(OpenSocket, int*(void *context, int type) );
	MOCK_METHOD1(CloseSocket, int(void *socket) );
	MOCK_METHOD2(MsgInitSize, int(void *zmq_msg, size_t size) );
	MOCK_METHOD1(MsgInit, int(void *zmq_msg) );
	MOCK_METHOD1(MsgData, int*(void *zmq_msg) );
	MOCK_CONST_METHOD1(MsgSize, size_t(void *zmq_msg) );
	MOCK_METHOD1(MsgClose, int(void *zmq_msg) );
	MOCK_CONST_METHOD0(ErrNo, int() );
	MOCK_METHOD2(Realloc, int*(void *ptr, size_t size) );
	MOCK_METHOD1(Malloc, int*(size_t size) );
	MOCK_METHOD2(Calloc, int*(size_t nmemb, size_t size) );
};


class ZeromqMockLayer{
public:
	static void Set(ZeromqAbstract *mock ){ abstract_mock = mock; }
public:
	static void* init(int io_threads){
		return (void*)abstract_mock->Init(io_threads);
	}
	static int term(void *context){
		return abstract_mock->Term(context);
	}
	static int send(void *socket, void *zmq_msg, int flags){
		return abstract_mock->Send(socket, zmq_msg, flags);
	}
	static int recv(void *s, void *zmq_msg, int flags){
		return abstract_mock->Recv(s, zmq_msg, flags);
	}
	static int bind(void *s, const char *addr){
		return abstract_mock->Bind(s, addr);
	}
	static int connect(void *s, const char *addr){
		return abstract_mock->Connect(s, addr);
	}
	static void* open_socket(void *context, int type){
		return (void*)abstract_mock->OpenSocket(context, type);
	}
	static int close_socket(void *s){
		return abstract_mock->CloseSocket(s);
	}
	static int msg_init_size(void *zmq_msg, size_t size){
		return abstract_mock->MsgInitSize(zmq_msg, size);
	}
	static int msg_init(void *zmq_msg){
		return abstract_mock->MsgInit(zmq_msg);
	}
	static void* msg_data(void *zmq_msg){
		return (void*)abstract_mock->MsgData(zmq_msg);
	}
	static size_t msg_size(void *zmq_msg){
		return abstract_mock->MsgSize(zmq_msg);
	}
	static int msg_close(void *zmq_msg){
		return abstract_mock->MsgClose(zmq_msg);
	}
	static int errno_io(void){
		return abstract_mock->ErrNo();
	}
	static const char *str_error(int errnum){
		return __empty_str;
	}
	static void *realloc(void *ptr, size_t size){
		return abstract_mock->Realloc(ptr, size);
	}
	static void *malloc(size_t size){
		return abstract_mock->Malloc(size);
	}
	static void *calloc(size_t nmemb, size_t size){
		return abstract_mock->Calloc(nmemb, size);
	}

public:
	static ZeromqAbstract *abstract_mock;
};

ZeromqAbstract *ZeromqMockLayer::abstract_mock=NULL;


// Test harness for routines in zmq_netw.c
class ZmqNetwMockTests : public ::testing::Test {
public:
	ZmqNetwMockTests(){}
	~ZmqNetwMockTests(){ delete io_mock; }
	static void SetMockInterface( struct zeromq_interface* io_if );
	void FaultInjectionForMalloc(struct zeromq_interface* io_if, int enable);
	void FaultInjectionForRealloc(struct zeromq_interface* io_if, int enable);
	void FaultInjectionForCalloc(struct zeromq_interface* io_if, int enable);
	virtual void SetUp();
	virtual void TearDown();
protected:
	ZeromqAbstract           *io_mock;
	struct zeromq_interface io_interface;
};

void ZmqNetwMockTests::SetMockInterface( struct zeromq_interface* io_if ){
	io_if->init = ZeromqMockLayer::init;
	io_if->term = ZeromqMockLayer::term;
	io_if->send = ZeromqMockLayer::send;
	io_if->recv = ZeromqMockLayer::recv;
	io_if->bind = ZeromqMockLayer::bind;
	io_if->connect = ZeromqMockLayer::connect;
	io_if->open_socket = ZeromqMockLayer::open_socket;
	io_if->close_socket = ZeromqMockLayer::close_socket;
	io_if->msg_init_size = ZeromqMockLayer::msg_init_size;
	io_if->msg_init = ZeromqMockLayer::msg_init;
	io_if->msg_data = ZeromqMockLayer::msg_data;
	io_if->msg_size = ZeromqMockLayer::msg_size;
	io_if->msg_close = ZeromqMockLayer::msg_close;
	io_if->errno_io = ZeromqMockLayer::errno_io;
	io_if->strerror = ZeromqMockLayer::str_error;
	/*by default standard memory management used*/
	io_if->realloc  = realloc;
	io_if->malloc  = malloc;
	io_if->calloc = calloc;
}

void ZmqNetwMockTests::SetUp() {
	io_mock = new MockZeromq;
	ZeromqMockLayer::Set(io_mock);
	SetMockInterface(&io_interface);
}

void ZmqNetwMockTests::TearDown() {
	delete io_mock, io_mock = NULL;
}

void ZmqNetwMockTests::FaultInjectionForMalloc(struct zeromq_interface* io_if, int enable){
	if (enable)
		io_if->malloc  = ZeromqMockLayer::malloc;
	else
		io_if->malloc  = malloc;
}

void ZmqNetwMockTests::FaultInjectionForRealloc(struct zeromq_interface* io_if, int enable){
	if (enable)
		io_if->realloc  = ZeromqMockLayer::realloc;
	else
		io_if->realloc  = realloc;
}

void ZmqNetwMockTests::FaultInjectionForCalloc(struct zeromq_interface* io_if, int enable){
	if (enable)
		io_if->calloc  = ZeromqMockLayer::calloc;
	else
		io_if->calloc  = calloc;
}

void TestCloseAllOk( ZeromqAbstract* io, struct zeromq_pool* pool, struct db_records_t db_records )
{
	int not_null_socket(0);
	InSequence dummy;
	for ( int i=0; i < db_records.count; i++ ){
		struct db_record_t *record = &db_records.array[i];
		struct sock_file_t *sockf = sockf_by_fd(pool, record->fd);
		if ( sockf ){
			if ( 'w' == record->fmode ){
				EXPECT_CALL(*(MockZeromq*)io, MsgInit(_)).WillRepeatedly(Return(0));
				EXPECT_CALL(*(MockZeromq*)io, Recv(_, _, ZMQ_NOBLOCK)).WillRepeatedly(Return(0));
				EXPECT_CALL(*(MockZeromq*)io, MsgClose(_)).WillRepeatedly(Return(0));
				EXPECT_CALL(*(MockZeromq*)io, CloseSocket(_)).WillRepeatedly(Return(0));
			}
			else if ( 'r' == record->fmode ){
				/*close REQ socket*/
				EXPECT_CALL(*(MockZeromq*)io, CloseSocket(&not_null_socket)).WillRepeatedly(Return(0));
			}
		}
	}
	/*close all sockets*/
	EXPECT_EQ( 0, close_all_comm_files( pool ) );
}


void TestOpenAllOk( ZeromqAbstract* io, struct zeromq_pool* pool, struct db_records_t db_records )
{
	int not_null_socket(0);
	InSequence dummy;
	for ( int i=0; i < db_records.count; i++ ){
		struct db_record_t *record = &db_records.array[i];
		if ( 'w' == record->fmode ){
			/*open REP socket*/
			EXPECT_CALL(*(MockZeromq*)io, OpenSocket(pool->context, ZMQ_REP)).WillRepeatedly(Return(&not_null_socket));
			EXPECT_CALL(*(MockZeromq*)io, Bind(_, _)).WillRepeatedly(Return(0));

		}
		else if ( 'r' == record->fmode ){
			/*open REQ socket*/
			EXPECT_CALL(*(MockZeromq*)io, OpenSocket(pool->context, ZMQ_REQ)).WillRepeatedly(Return(&not_null_socket));
			EXPECT_CALL(*(MockZeromq*)io, Connect(_, _)).WillRepeatedly(Return(0));
		}
	}
	/*open all sockets*/
	EXPECT_EQ( 0, open_all_comm_files( pool, &db_records) );
}


TEST_F(ZmqNetwMockTests, TestInitTermZeromqPool1) {
	struct zeromq_pool * pool = (struct zeromq_pool*) malloc(sizeof(struct zeromq_pool));
	assert(pool);

	/*test case : init context failed*/
	EXPECT_CALL(*(MockZeromq*)io_mock, Init(_)).WillOnce(ReturnNull());
	EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
	EXPECT_EQ( LOAD_0MQ_CONTEXT_INIT_FAILED, init_zeromq_pool( &io_interface, pool) );

	int context(0);
	struct zeromq_interface *interf = NULL;
	struct zeromq_pool * foo_pool = NULL;
	/*test case: bad parameters*/
	ASSERT_DEATH(init_zeromq_pool( NULL, foo_pool ), "");
	ASSERT_DEATH(init_zeromq_pool( interf, NULL ), "");

	/*test case : init context OK*/
	EXPECT_CALL(*(MockZeromq*)io_mock, Init(_)).WillOnce(Return(&context));
	EXPECT_EQ( LOAD_OK, init_zeromq_pool( &io_interface, pool) );

	EXPECT_CALL(*(MockZeromq*)io_mock, Term(pool->context)).WillOnce(Return(-1));
	EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
	EXPECT_EQ( LOAD_0MQ_CONTEXT_TERM_FAILED, zeromq_term( pool) );

	ASSERT_DEATH(zeromq_term( NULL ), "");

	pool->context = NULL;
	ASSERT_DEATH(zeromq_term( pool ), "");

	free(pool);
}


TEST_F(ZmqNetwMockTests, TestSockfArray) {
	struct sock_file_t sockf;
	struct zeromq_pool * pool = (struct zeromq_pool*) malloc(sizeof(struct zeromq_pool));
	assert(pool);

	int context(0);
	EXPECT_CALL(*(MockZeromq*)io_mock, Init(_)).WillOnce(Return(&context));
	EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
	EXPECT_EQ( LOAD_OK, init_zeromq_pool( &io_interface, pool) );

	/*test case: dynamic array growing ok - add many items */
	for (int i=0; i < 100; i++){
		memset(&sockf, '\0', sizeof(struct sock_file_t));
		sockf.fs_fd = i;
		int err = add_sockf_copy_to_array( pool, &sockf);
		if ( LOAD_OK != err ){
			FAIL();
		}
	}

	//test array : add duplicated items
	sockf.fs_fd = 800;
	EXPECT_EQ( LOAD_OK, add_sockf_copy_to_array( pool, &sockf) );
	sockf.fs_fd = 800;
	EXPECT_EQ( LOAD_ITEM_ALREADY_EXIST, add_sockf_copy_to_array( pool, &sockf) );

	EXPECT_CALL(*(MockZeromq*)io_mock, Term(pool->context)).WillOnce(Return(0));
	EXPECT_EQ( LOAD_OK, zeromq_term( pool) );
	free(pool);
}


TEST_F(ZmqNetwMockTests, TestOpenCloseSockf) {
	struct zeromq_pool * pool = (struct zeromq_pool*) malloc(sizeof(struct zeromq_pool));
	assert(pool);
	int context(0);
	{
		InSequence dummy;
		EXPECT_CALL(*(MockZeromq*)io_mock, Init(_)).WillOnce(Return(&context));
		EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
		EXPECT_EQ( LOAD_OK, init_zeromq_pool( &io_interface, pool) );
	}

	/*zmq init ok, create fake db_records struct*/
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	db_records.array = (struct db_record_t*)malloc( sizeof(struct db_record_t)*db_records.maxcount );
	memset(db_records.array, '\0', sizeof(struct db_record_t)*db_records.maxcount);
	struct db_record_t *record = NULL;
	/*add test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 3;
	record->fmode = 'w';
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*add test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 4;
	record->fmode = 'r';
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*add bad test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 5;
	record->fmode = 'x'; //Wrong mode not read / not write
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*add bad test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 6;
	record->fmode = 'w'; //Wrong mode not read / not write
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_UNKNOWN;

	/*records added*/
	/*open, close socket using good data for optimistic scenary*/
	struct sock_file_t* sockf = NULL;
	int not_null_socket(0);
	/*test case: error allocate sockf in heap, use malloc fault injection*/
	{
		FaultInjectionForCalloc(&io_interface, ENABLE);
		EXPECT_CALL(*(MockZeromq*)io_mock, Calloc(_,sizeof(struct sock_file_t))).WillOnce(ReturnNull());
		EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 3));
		FaultInjectionForCalloc(&io_interface, DISABLE);
	}

	/*test REP socket: fd=3*/
	{
		//open socket
		EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(pool->context, ZMQ_REP)).WillOnce(Return(&not_null_socket));
		EXPECT_CALL(*(MockZeromq*)io_mock, Bind(_, _)).WillOnce(Return(0));

		/*open socket*/
		sockf = open_sockf( pool, &db_records, 3);
		EXPECT_NE( (struct sock_file_t*)NULL, sockf);
		/*try open twice the same file descriptor socket: return already opened socket
		 * should no calls to zmq */
		sockf = open_sockf( pool, &db_records, 3);
		EXPECT_NE( (struct sock_file_t*)NULL, sockf );
	}
	{
		//close socket 3
		InSequence dummy;
		EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(&not_null_socket)).WillOnce(Return(0));
		EXPECT_EQ( 0, close_sockf( pool, sockf) );
	}
	{
		/*test open socket REP error*/
		InSequence dummy;
		EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(pool->context, ZMQ_REP)).WillOnce(ReturnNull());
		EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
		EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 3) );
	}
	{
		/*test bind socket error*/
		InSequence dummy;
		EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(_, _)).WillOnce(Return(&not_null_socket));
		EXPECT_CALL(*(MockZeromq*)io_mock, Bind(_, _)).WillOnce(Return(-1));
		EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
		EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(_)).Times(1);
		EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 3) );
	}
	/*test connect socket error*/
	EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(_, _)).WillOnce(Return(&not_null_socket));
	EXPECT_CALL(*(MockZeromq*)io_mock, Connect(_, _)).WillOnce(Return(-1));
	EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
	EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(_)).Times(1);
	EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 4) );

	/*test socket open : fd=5 with wrong fmode*/
	EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 5) );

	/*test socket open : fd=5 with wrong socket type*/
	EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( pool, &db_records, 6) );

	EXPECT_CALL(*(MockZeromq*)io_mock, Term(pool->context)).WillOnce(Return(0));
	EXPECT_EQ( LOAD_OK, zeromq_term( pool) );
	free(pool);
	free(db_records.array);
}


TEST_F(ZmqNetwMockTests, TestOpenReadWriteCloseAll) {
	struct zeromq_pool * pool = (struct zeromq_pool*) malloc(sizeof(struct zeromq_pool));
	assert(pool);
	int context(0);
	{
		InSequence dummy;
		EXPECT_CALL(*(MockZeromq*)io_mock, Init(_)).WillOnce(Return(&context));
		EXPECT_CALL(*(MockZeromq*)io_mock, ErrNo()).Times(AnyNumber());
		EXPECT_EQ( LOAD_OK, init_zeromq_pool( &io_interface, pool ) );
	}

	/*zmq init ok, create fake db_records struct*/
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	db_records.array = (struct db_record_t*)malloc( sizeof(struct db_record_t)*db_records.maxcount );
	memset(db_records.array, '\0', sizeof(struct db_record_t)*db_records.maxcount);
	struct db_record_t *record = NULL;
	/*add test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 3;
	record->fmode = 'w';
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*add test item into db_records*/
	record = &db_records.array[db_records.count++];
	record->fd = 4;
	record->fmode = 'r';
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*records added*/

	int not_null_socket(0);
	/*test case 1: open all failed; Error injection on bind*/
	{
		InSequence dummy;
		for ( int i=0; i < db_records.count; i++ ){
			struct db_record_t *record = &db_records.array[i];
			if ( 'w' == record->fmode ){
				/*open REP socket*/
				EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(pool->context, ZMQ_REP)).WillOnce(Return(&not_null_socket));
				EXPECT_CALL(*(MockZeromq*)io_mock, Bind(_, _)).WillOnce(Return(-1));
				EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(&not_null_socket)).WillOnce(Return(-1));
			}
		}
		/*open all sockets*/
		EXPECT_EQ( LOAD_0MQ_SOCKET_ERROR, open_all_comm_files( pool, &db_records) );
	}

	/*test case 2: open all failed; Error injection on 2nd openSocket*/
	{
		InSequence dummy;
		for ( int i=0; i < db_records.count; i++ ){
			struct db_record_t *record = &db_records.array[i];
			if ( 'w' == record->fmode ){
				/*open REP socket*/
				EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(pool->context, ZMQ_REP)).WillOnce(Return(&not_null_socket));
				EXPECT_CALL(*(MockZeromq*)io_mock, Bind(_, _)).WillOnce(Return(0));
			}
			else if ( 'r' == record->fmode ){
				/*open REQ socket*/
				EXPECT_CALL(*(MockZeromq*)io_mock, OpenSocket(pool->context, ZMQ_REQ)).WillOnce(ReturnNull());
			}
		}
		/*open all sockets*/
		EXPECT_EQ( LOAD_0MQ_SOCKET_ERROR, open_all_comm_files( pool, &db_records) );
	}
	/*close all to continue tests*/
	TestCloseAllOk( io_mock, pool, db_records );

	/*test case: open all OK*/
	TestOpenAllOk( io_mock, pool, db_records );

	char buf[10];
	/*Read tests begin*/
	{
		InSequence dummy;
		for ( int i=0; i < db_records.count; i++ ){
			struct db_record_t *record = &db_records.array[i];
			struct sock_file_t *sockf = sockf_by_fd(pool, record->fd);
			if ( 'w' == record->fmode ){
				/*test case: Send ok*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInitSize(_, 10)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgData(_)).WillOnce(Return((int*)buf));
				EXPECT_CALL(*(MockZeromq*)io_mock, Send(sockf->netw_socket, _, 0)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillOnce(Return(0));
				EXPECT_EQ( 10, write_sockf( sockf, buf, 10) );
				/*test case: MsgInitSize failed*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInitSize(_, 10)).WillOnce(Return(-1));
				EXPECT_EQ( -1, write_sockf( sockf, buf, 10) );
				/*test case: Send failed*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInitSize(_, 10)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgData(_)).WillOnce(Return((int*)buf));
				EXPECT_CALL(*(MockZeromq*)io_mock, Send(sockf->netw_socket, _, 0)).WillOnce(Return(-1));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillOnce(Return(0));
				EXPECT_EQ( -1, write_sockf( sockf, buf, 10) );
			}
			else if ( 'r' == record->fmode ){
				/*test case: Recv ok*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInit(_)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, Recv(sockf->netw_socket, _, 0)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgSize(_)).WillOnce(Return(10));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgData(_)).WillOnce(Return((int*)buf));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillOnce(Return(0));
				EXPECT_EQ( 10, read_sockf( sockf, buf, 10) );
				/*test case: Recv failed*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInit(_)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, Recv(sockf->netw_socket, _, 0)).WillOnce(Return(-1));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillOnce(Return(0));
				EXPECT_EQ( -1, read_sockf( sockf, buf, 10) );
				/*test case: Send failed*/
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgInitSize(_, 10)).WillOnce(Return(0));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgData(_)).WillOnce(Return((int*)buf));
				EXPECT_CALL(*(MockZeromq*)io_mock, Send(sockf->netw_socket, _, 0)).WillOnce(Return(-1));
				EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillOnce(Return(0));
				EXPECT_EQ( -1, write_sockf( sockf, buf, 10) );
			}
		}
	}

	/*test case: close all failed; Error injection on closeSocket*/
	{
		int not_null_socket(0);
		InSequence dummy;
		for ( int i=0; i < db_records.count; i++ ){
			struct db_record_t *record = &db_records.array[i];
			struct sock_file_t *sockf = sockf_by_fd(pool, record->fd);
			if ( sockf ){
				if ( 'w' == record->fmode ){
					EXPECT_CALL(*(MockZeromq*)io_mock, MsgInit(_)).WillRepeatedly(Return(0));
					EXPECT_CALL(*(MockZeromq*)io_mock, Recv(_, _, ZMQ_NOBLOCK)).WillRepeatedly(Return(0));
					EXPECT_CALL(*(MockZeromq*)io_mock, MsgClose(_)).WillRepeatedly(Return(0));
					EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(_)).WillRepeatedly(Return(-1));
				}
				else if ( 'r' == record->fmode ){
					/*close REQ socket*/
					EXPECT_CALL(*(MockZeromq*)io_mock, CloseSocket(&not_null_socket)).WillRepeatedly(Return(-1));
				}
			}
		}
		/*close all sockets*/
		EXPECT_EQ( LOAD_0MQ_SOCKET_ERROR, close_all_comm_files( pool ) );
	}

	EXPECT_CALL(*(MockZeromq*)io_mock, Term(pool->context)).WillOnce(Return(0));
	EXPECT_EQ( LOAD_OK, zeromq_term( pool) );
	free(pool);
	free(db_records.array);
}


int main(int argc, char **argv) {
	testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}



