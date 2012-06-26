/*
 * Tests for zerovm network protocol layer
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>
#include <zmq.h>

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "src/service_runtime/dyn_array.h"
#include "src/service_runtime/nacl_error_code.h"
#include "src/platform/nacl_log.h"
extern "C" {
#include "src/networking/zmq_netw.h"
#include "src/networking/zvm_netw.h"
}
#define ENABLE 1
#define DISABLE 0

static struct sock_file_t __sockf1;
static struct sock_file_t __sockf2;
static struct sock_file_t* __local_sockf_array[2];
static struct DynArray __fakeArray;
static struct zvm_netw_header_t __header = DEFAULT_ZVM_NETW_HEAD;

using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::InSequence;
using ::testing::AnyNumber;
using ::testing::Return;
using ::testing::ReturnPointee;
using ::testing::ReturnNull;
using ::testing::AtLeast;
using ::testing::_;

class ZmqNetwAbstract {
public:
	virtual ~ZmqNetwAbstract(){}
	virtual int GetAllDbRecords(const char *path, const char *nodename, struct db_records_t *db_records) = 0;
	virtual struct db_record_t *MatchDbRecordByFd(struct db_records_t *records, int fd) = 0;
	virtual uint64_t GetFileSize(const char *name)= 0;
	virtual struct sock_file_t* SockfByFd(struct zeromq_pool* zpool, int fd) = 0;
	virtual int InitZeromqPool(struct zeromq_interface* io_interface, struct zeromq_pool * zpool) = 0;
	virtual int OpenAllCommFiles(struct zeromq_pool* zpool, struct db_records_t *db_records) = 0;
	virtual int CloseAllCommFiles(struct zeromq_pool* zpool) = 0;
	virtual int ZeromqTerm(struct zeromq_pool* zpool) = 0;
	virtual ssize_t WriteSockf(struct sock_file_t *sockf, const char *buf, size_t size) = 0;
	virtual ssize_t ReadSockf(struct sock_file_t *sockf, char *buf, size_t size) = 0;
};


class MockZmqNetw : public ZmqNetwAbstract {
public:
	MOCK_METHOD3(GetAllDbRecords, int(const char *path, const char *nodename, struct db_records_t *db_records) );
	MOCK_METHOD2(MatchDbRecordByFd, struct db_record_t *(struct db_records_t *records, int fd) );
	MOCK_METHOD1(GetFileSize, uint64_t(const char *name) );
	MOCK_METHOD2(SockfByFd, struct sock_file_t*(struct zeromq_pool* zpool, int fd) );
	MOCK_METHOD2(InitZeromqPool, int(struct zeromq_interface* io_interface, struct zeromq_pool * zpool) );
	MOCK_METHOD2(OpenAllCommFiles, int(struct zeromq_pool* zpool, struct db_records_t *db_records) );
	MOCK_METHOD1(CloseAllCommFiles, int(struct zeromq_pool* zpool) );
	MOCK_METHOD1(ZeromqTerm, int(struct zeromq_pool* zpool) );
	MOCK_METHOD3(WriteSockf, ssize_t(struct sock_file_t *sockf, const char *buf, size_t size) );
	MOCK_METHOD3(ReadSockf, ssize_t(struct sock_file_t *sockf, char *buf, size_t size) );
};


class ZmqNetwMockLayer{
public:
	static void Set(ZmqNetwAbstract *mock ){ abstract_mock = mock; }
public:
	static int GetAllDbRecords(const char *path, const char *nodename, struct db_records_t *db_records){
		return abstract_mock->GetAllDbRecords(path, nodename, db_records);
	}
	static struct db_record_t *MatchDbRecordByFd(struct db_records_t *records, int fd){
		return abstract_mock->MatchDbRecordByFd(records, fd);
	}
	static uint64_t GetFileSize(const char *name){
		return abstract_mock->GetFileSize(name);
	}
	static struct sock_file_t* SockfByFd(struct zeromq_pool* zpool, int fd){
		return abstract_mock->SockfByFd(zpool, fd);
	}
	static int InitZeromqPool(struct zeromq_interface* io_interface, struct zeromq_pool * zpool){
		return abstract_mock->InitZeromqPool(io_interface, zpool);
	}
	static int OpenAllCommFiles(struct zeromq_pool* zpool, struct db_records_t *db_records){
		return abstract_mock->OpenAllCommFiles(zpool, db_records);
	}
	static int CloseAllCommFiles(struct zeromq_pool* zpool){
		return abstract_mock->CloseAllCommFiles(zpool);
	}
	static int ZeromqTerm(struct zeromq_pool* zpool){
		return abstract_mock->ZeromqTerm(zpool);
	}
	static ssize_t WriteSockf(struct sock_file_t *sockf, const char *buf, size_t size){
		return abstract_mock->WriteSockf(sockf, buf, size);
	}
	static ssize_t ReadSockf(struct sock_file_t *sockf, char *buf, size_t size){
		return abstract_mock->ReadSockf(sockf, buf, size);
	}
	static ssize_t SetArg1ReadSockf(struct sock_file_t *sockf, char *buf, size_t size){
		struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
		header.command = COMMAND_FIN;
		strncpy(buf, (const char*)&header, sizeof(struct zvm_netw_header_t));
		return size;
	}

public:
	static ZmqNetwAbstract *abstract_mock;
};

ZmqNetwAbstract *ZmqNetwMockLayer::abstract_mock=NULL;


ACTION_TEMPLATE(
    StrCpyToArg,
    HAS_1_TEMPLATE_PARAMS(int, k),
    AND_1_VALUE_PARAMS(str)) {
  strcpy(std::tr1::get<k>(args), str);
}

// Test harness for routines in zmq_netw.c
class ZvmNetwMockTests : public ::testing::Test {
public:
	ZvmNetwMockTests(){}
	~ZvmNetwMockTests(){ delete io_mock; }
	static void SetMockInterface( struct zmq_netw_interface* io_if );
	void SetArg1ReadSockf(struct zmq_netw_interface* io_if, int enable);
	//void FaultInjectionForMalloc(struct zeromq_interface* io_if, int enable);
	//void FaultInjectionForRealloc(struct zeromq_interface* io_if, int enable);
	virtual void SetUp();
	virtual void TearDown();
	void InitConfig();
	void TermConfig();
protected:
	ZmqNetwAbstract           *io_mock;
	struct zmq_netw_interface io_interface;
};

void ZvmNetwMockTests::SetMockInterface( struct zmq_netw_interface* io_if ){
	io_if->get_all_dbrecords = ZmqNetwMockLayer::GetAllDbRecords;
	io_if->match_db_record_by_fd = ZmqNetwMockLayer::MatchDbRecordByFd;
	io_if->GetFileSize = ZmqNetwMockLayer::GetFileSize;
	io_if->sockf_by_fd = ZmqNetwMockLayer::SockfByFd;
	io_if->init_zeromq_pool = ZmqNetwMockLayer::InitZeromqPool;
	io_if->open_all_comm_files = ZmqNetwMockLayer::OpenAllCommFiles;
	io_if->close_all_comm_files = ZmqNetwMockLayer::CloseAllCommFiles;
	io_if->zeromq_term = ZmqNetwMockLayer::ZeromqTerm;
	io_if->write_sockf = ZmqNetwMockLayer::WriteSockf;
	io_if->read_sockf = ZmqNetwMockLayer::ReadSockf;
}

void ZvmNetwMockTests::SetUp() {
	io_mock = new MockZmqNetw;
	ZmqNetwMockLayer::Set(io_mock);
	SetMockInterface(&io_interface);
}

void ZvmNetwMockTests::TearDown() {
	delete io_mock, io_mock = NULL;
}


void ZvmNetwMockTests::SetArg1ReadSockf(struct zmq_netw_interface* io_if, int enable){
	if ( enable )
		io_if->read_sockf = ZmqNetwMockLayer::SetArg1ReadSockf;
	else
		io_if->read_sockf = ZmqNetwMockLayer::ReadSockf;
}


void ZvmNetwMockTests::InitConfig(){
	/*preconfiguration before tests*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetFileSize(_)).WillOnce(Return(1));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetAllDbRecords(_, _, _)).WillOnce( Return(LOAD_OK) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, InitZeromqPool(_, _)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, OpenAllCommFiles(_, _)).WillOnce(Return(LOAD_OK));
	EXPECT_EQ( LOAD_OK, init_zvm_networking(&io_interface, "fake dbname", "fake nodename", 1) );
	__fakeArray.num_entries = 2;
	struct zeromq_pool *zpool = pool_for_unit_test();
	zpool->sockf_array = &__fakeArray;
	zpool->sockf_array->ptr_array = (void**)__local_sockf_array;
	struct sock_file_t sockf1 = {&__fakeArray, ESOCKET_REQREP, 'w', EREADWRITE, 3 };
	__sockf1 = sockf1;
	struct sock_file_t sockf2 = {&__fakeArray, ESOCKET_REQREP, 'r', EREADWRITE, 4 };
	__sockf2 = sockf2;
	zpool->sockf_array->ptr_array[0] = &__sockf1;
	zpool->sockf_array->ptr_array[1] = &__sockf2;
}

void ZvmNetwMockTests::TermConfig(){
	/*test case: term ok*/
	SetArg1ReadSockf(&io_interface, ENABLE);
	struct db_records_t *db_records = db_records_for_unit_test();
	struct db_record_t db_record;
	db_records->count++;
	db_record.endpoint = (char*)malloc(10); /*it will be destroyed in zeromq_term*/
	db_record.nodename = (char*)malloc(10); /*it will be destroyed in zeromq_term*/
	db_records->array = &db_record;
	EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&__sockf2, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&__sockf1, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, CloseAllCommFiles(_)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ZeromqTerm(_)).WillOnce(Return(LOAD_OK));
	EXPECT_EQ( LOAD_OK, term_zvm_networking() );
	SetArg1ReadSockf(&io_interface, DISABLE);
}


TEST_F(ZvmNetwMockTests, TestInitZvmNetworking) {
	/*test case: database does not exist, fault injection used*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetFileSize(_)).WillOnce(Return(0));
	EXPECT_EQ( LOAD_OPEN_ERROR, init_zvm_networking(&io_interface, "fake dbname", "fake nodename", 1) );
	/*test case: database sql statement exec error*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetFileSize(_)).WillOnce(Return(1));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetAllDbRecords(_, _, _)).WillOnce(Return(LOAD_SQL_STATEMENT_EXEC_ERROR));
	EXPECT_EQ( LOAD_SQL_STATEMENT_EXEC_ERROR, init_zvm_networking(&io_interface, "fake dbname", "fake nodename", 1) );
	/*test case: init_zeromq_pool error*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetFileSize(_)).WillOnce(Return(1));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetAllDbRecords(_, _, _)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, InitZeromqPool(_, _)).WillOnce(Return(LOAD_0MQ_CONTEXT_INIT_FAILED));
	EXPECT_EQ( LOAD_0MQ_CONTEXT_INIT_FAILED, init_zvm_networking(&io_interface, "fake dbname", "fake nodename", 1) );
	/*test case: open_all_comm_files error*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetFileSize(_)).WillOnce(Return(1));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, GetAllDbRecords(_, _, _)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, InitZeromqPool(_, _)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, OpenAllCommFiles(_, _)).WillOnce(Return(LOAD_0MQ_SOCKET_ERROR));
	EXPECT_EQ( LOAD_0MQ_SOCKET_ERROR, init_zvm_networking(&io_interface, "fake dbname", "fake nodename", 1) );
}


TEST_F(ZvmNetwMockTests, TestTermZvmNetworking) {
	InitConfig();
	/*test case: close_all_comm_files error*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&__sockf2, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&__sockf1, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, CloseAllCommFiles(_)).WillOnce(Return(LOAD_0MQ_SOCKET_ERROR));
	EXPECT_EQ( LOAD_0MQ_SOCKET_ERROR, term_zvm_networking() );

	InitConfig();
	/*test case: zeromq_term error*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&__sockf2, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&__sockf1, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, CloseAllCommFiles(_)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ZeromqTerm(_)).WillOnce(Return(LOAD_0MQ_CONTEXT_TERM_FAILED));
	EXPECT_EQ( LOAD_0MQ_CONTEXT_TERM_FAILED, term_zvm_networking() );

	InitConfig();
	/*test case: Applicative network error*/
	__header.protoid = PROTOID;
	struct db_records_t *db_records = db_records_for_unit_test();
	struct db_record_t db_record;
	db_records->count++;
	db_record.endpoint = (char*)malloc(10); /*it will be destroyed in zeromq_term*/
	db_record.nodename = (char*)malloc(10); /*it will be destroyed in zeromq_term*/
	db_records->array = &db_record;
	EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&__sockf2, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&__sockf1, _, _))
		.WillRepeatedly( Return( sizeof(struct zvm_netw_header_t) ) );
	EXPECT_CALL(*(MockZmqNetw*)io_mock, CloseAllCommFiles(_)).WillOnce(Return(LOAD_OK));
	EXPECT_CALL(*(MockZmqNetw*)io_mock, ZeromqTerm(_)).WillOnce(Return(LOAD_OK));
	EXPECT_EQ( LOAD_APPLICATIVE_NETWORK_ERROR, term_zvm_networking() );

}


TEST_F(ZvmNetwMockTests, TestCapabilitiesForFile) {
	struct db_record_t db_record;
	/*test case: unknown descriptor*/
	EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,1)).WillOnce( ReturnNull() );
	EXPECT_EQ( ENOTALLOWED, capabilities_for_file_fd(1) );
	/*test case: EREAD*/
	db_record.fmode = 'r';
	EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,1)).WillOnce( Return(&db_record) );
	EXPECT_EQ( EREAD, capabilities_for_file_fd(1) );
	/*test case: EWRITE*/
	db_record.fmode = 'w';
	EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,1)).WillOnce( Return(&db_record) );
	EXPECT_EQ( EWRITE, capabilities_for_file_fd(1) );

}

TEST_F(ZvmNetwMockTests, TestCommfRead) {
	InitConfig();
	size_t count(0);
	struct sock_file_t sockf;
	sockf.sock_type = ESOCKET_UNKNOWN;
	struct db_record_t db_record;
	char buf;
	{
		/*test case: unknown descriptor*/
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( ReturnNull() );
		EXPECT_EQ( -1, commf_read(1, &buf, count) );
	}
	db_record.fmode = 'r';
	{
		/*test case: sockf_by_fd not found*/
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( ReturnNull() );
		EXPECT_EQ( -1, commf_read(1,NULL,1) );
	}
	{
		/*test case: unknown socket type*/
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( Return(&sockf) );
		EXPECT_EQ( -1, commf_read(1,NULL,1) );
	}
	{
		/*test case: pull data*/
		sockf.sock_type = ESOCKET_REQREP;
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( Return(&sockf) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&sockf, _, sizeof(struct zvm_netw_header_t) ))
			.WillOnce( Return(sizeof(struct zvm_netw_header_t)) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&sockf, _, _ )).WillOnce( Return(2) );
		EXPECT_EQ( 2, commf_read(1,NULL,2) );
	}
	TermConfig();
}

TEST_F(ZvmNetwMockTests, TestCommfWrite) {
	InitConfig();
	size_t count(0);
	struct sock_file_t sockf;
	sockf.sock_type = ESOCKET_UNKNOWN;
	struct db_record_t db_record;
	const char buf=0;
	{
		/*test case: unknown descriptor*/
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( ReturnNull() );
		EXPECT_EQ( -1, commf_write(1, &buf, count) );
	}
	db_record.fmode = 'w';
	{
		/*test case: sockf_by_fd not found*/

		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( ReturnNull() );
		EXPECT_EQ( -1, commf_write(1,NULL,1) );
	}
	{
		/*test case: unknown socket type*/
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( Return(&sockf) );
		EXPECT_EQ( -1, commf_write(1,NULL,1) );
	}
	{
		/*test case: pull data*/
		sockf.sock_type = ESOCKET_REQREP;
		EXPECT_CALL(*(MockZmqNetw*)io_mock, MatchDbRecordByFd(_,_)).WillOnce( Return(&db_record) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, SockfByFd(pool_for_unit_test(), _ )).WillOnce( Return(&sockf) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, ReadSockf(&sockf, _, sizeof(struct zvm_netw_header_t) ))
			.WillOnce( Return(sizeof(struct zvm_netw_header_t)) );
		EXPECT_CALL(*(MockZmqNetw*)io_mock, WriteSockf(&sockf, _, _ )).WillOnce( Return(0) );
		EXPECT_EQ( 0, commf_write(1,NULL,0) );
	}
	TermConfig();
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
