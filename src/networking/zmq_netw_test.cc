/*
 * Tests for zeromq networking
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>
#include <zmq.h>

#include "gtest/gtest.h"
#include "src/platform/nacl_log.h"
extern "C" {
#include "src/networking/errcodes.h"
#include "src/networking/zmq_netw.h"
#include "src/networking/sqluse_srv.h"
}

const char *__endpoint1 = "ipc:///tmp/test1\0";
#define TEST_DATA_SIZE 1000000
#define TEST_DB_PATH "gtest/data/zerovm_test.db"

/*helpers*/
struct zeromq_pool *CreateArrayPoolNoZmq();
void FreeArrayPoolNoZmq( struct zeromq_pool *);
char* alloc_fill_random(int bytes);


// Test harness for routines in zmq_netw.c
class ZmqNetwTests : public ::testing::Test {
public:
	ZmqNetwTests(){}
};


struct zeromq_pool *CreateArrayPoolNoZmq(){
	struct zeromq_pool * pool = (struct zeromq_pool*) malloc(sizeof(struct zeromq_pool));
	assert(pool);
	pool->count_max = ESOCKF_ARRAY_GRANULARITY;
	pool->sockf_array = (struct sock_file_t*) malloc(pool->count_max * sizeof(struct sock_file_t));
	EXPECT_NE(pool->sockf_array, (void*)NULL);
	if ( pool->sockf_array ) {
		memset(pool->sockf_array, '\0', sizeof(struct sock_file_t)*pool->count_max);
		for (int i=0; i < pool->count_max; i++)
			pool->sockf_array[i].unused = 1;
		return pool;
	}
	else{
		/*no memory allocated*/
		return NULL;
	}
}

void FreeArrayPoolNoZmq( struct zeromq_pool *pool){
	if ( pool ){
		if ( pool->sockf_array ){
			free(pool->sockf_array);
		}
		free(pool);
	}
}

char* alloc_fill_random(int bytes){
	char *data = (char*)malloc( bytes );
	if ( !data ) return data;
	pid_t pid = getpid();
	//fill array by random numbers
	srand((time_t)pid );
	for (int i=0; i<bytes; i++){
		data[i]=rand();
	}
	return data;

}

char* fill_by_random(char*buf, int bytes){
	/*fill data by randoms*/
	pid_t pid = getpid();
	srand((time_t)pid );
	for (int i=0; i<bytes; i++){
		buf[i]=rand();
	}
	return buf;
}

TEST_F(ZmqNetwTests, TestSockfArrayAdd100items) {
	struct zeromq_pool *pool = CreateArrayPoolNoZmq();
	EXPECT_NE((void*)NULL, pool);
	struct sock_file_t sockf;
	for (int i=0; i < 100; i++){
		memset(&sockf, '\0', sizeof(struct sock_file_t));
		sockf.fs_fd = i;
		int err = add_sockf_copy_to_array( pool, &sockf);
		if ( ERR_OK != err ){
			FAIL();
		}
	}
	FreeArrayPoolNoZmq(pool);
}

TEST_F(ZmqNetwTests, TestSockfArrayAddDuplicateditems) {
	struct zeromq_pool *pool = CreateArrayPoolNoZmq();
	EXPECT_NE((void*)NULL, pool);
	struct sock_file_t sockf;
	//add items
	sockf.fs_fd = 800;
	EXPECT_EQ( ERR_OK, add_sockf_copy_to_array( pool, &sockf) );
	//add duplicated items
	sockf.fs_fd = 800;
	EXPECT_EQ( ERR_ALREADY_EXIST, add_sockf_copy_to_array( pool, &sockf) );
	FreeArrayPoolNoZmq(pool);
}

TEST_F(ZmqNetwTests, TestSockfArrayFinditems) {
	struct zeromq_pool *pool = CreateArrayPoolNoZmq();
	EXPECT_NE((void*)NULL, pool);
	struct sock_file_t sockf;
	for (int i=0; i < 100; i++){
		memset(&sockf, '\0', sizeof(struct sock_file_t));
		sockf.fs_fd = i;
		int err = add_sockf_copy_to_array( pool, &sockf);
		if ( ERR_OK != err ){
			FAIL();
		}
	}
	EXPECT_EQ( (struct sock_file_t*)NULL, sockf_by_fd( pool, 101) );
	EXPECT_NE( (struct sock_file_t*)NULL, sockf_by_fd( pool, 1) );
	FreeArrayPoolNoZmq(pool);
}

TEST_F(ZmqNetwTests, TestInitTermZmqNetwPool) {
	struct zeromq_pool *zpool = (struct zeromq_pool *) malloc(sizeof(struct zeromq_pool *));
	EXPECT_EQ(ERR_OK, init_zeromq_pool(zpool) );
	EXPECT_EQ(ERR_OK, zeromq_term(zpool) );
	free(zpool);
}


TEST_F(ZmqNetwTests, TestSockfOpenClose) {
	struct zeromq_pool *zpool = (struct zeromq_pool *) malloc(sizeof(struct zeromq_pool *));
	EXPECT_EQ(ERR_OK, init_zeromq_pool(zpool) );
	/*zmq init ok, create db_records struct*/
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	db_records.array = (struct db_record_t*)malloc( sizeof(struct db_record_t)*db_records.maxcount );
	memset(db_records.array, '\0', sizeof(struct db_record_t)*db_records.maxcount);
	/*add test item into db_records*/
	struct db_record_t *record = &db_records.array[db_records.count++];
	/*set record data*/
	record->fd = 3;
	record->fmode = 'w';
	record->endpoint = (char*) __endpoint1;
	record->sock = ESOCKET_REQREP;
	/*records added*/
	/*open socket using good data*/
	struct sock_file_t* sockf = open_sockf( zpool, &db_records, 3);
	EXPECT_NE( (struct sock_file_t*)NULL, sockf);
	/*open twice the same file descriptor socket*/
	sockf = open_sockf( zpool, &db_records, 3);
	EXPECT_NE( (struct sock_file_t*)NULL, sockf );
	/*test close sockf*/
	EXPECT_EQ( ERR_OK, close_sockf(zpool, sockf) );
	/*close sockf twice*/
	EXPECT_EQ( ERR_OK, close_sockf(zpool, sockf) );
	EXPECT_EQ(ERR_OK, zeromq_term(zpool) );
	free(zpool);
}

TEST_F(ZmqNetwTests, TestSockfCommunicationReqRep) {
	struct zeromq_pool *zpool = (struct zeromq_pool *) malloc(sizeof(struct zeromq_pool *));
	EXPECT_EQ(ERR_OK, init_zeromq_pool(zpool) );
	/*zmq init ok, create db_records struct*/
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	db_records.array = (struct db_record_t*)malloc( sizeof(struct db_record_t)*db_records.maxcount );
	memset(db_records.array, '\0', sizeof(struct db_record_t)*db_records.maxcount);
	/*add test item into db_records*/
	struct db_record_t *record1 = &db_records.array[db_records.count++];
	/*set record1 data*/
	record1->fd = 3;
	record1->fmode = 'w';
	record1->endpoint = (char*)__endpoint1;
	record1->sock = ESOCKET_REQREP;
	struct db_record_t *record2 = &db_records.array[db_records.count++];
	/*set record2 data*/
	record2->fd = 4;
	record2->fmode = 'r';
	record2->endpoint = (char*)__endpoint1;
	record2->sock = ESOCKET_REQREP;
	/*records added*/
	struct sock_file_t* w_sockf = open_sockf( zpool, &db_records, 4);
	EXPECT_NE( (struct sock_file_t*)NULL, w_sockf);
	struct sock_file_t* r_sockf = open_sockf( zpool, &db_records, 3);
	EXPECT_NE( (struct sock_file_t*)NULL, r_sockf);
	char *buf = alloc_fill_random(TEST_DATA_SIZE);
	char *buf2 = (char*)malloc(TEST_DATA_SIZE);
	if ( buf ){
		/*write and read the same bytes count, and compare sent and recevied data*/
		EXPECT_EQ( TEST_DATA_SIZE, write_sockf(w_sockf, buf, TEST_DATA_SIZE) );
		EXPECT_EQ( TEST_DATA_SIZE, read_sockf(r_sockf, buf2, TEST_DATA_SIZE) );
		EXPECT_EQ( 0, strncmp(buf, buf2, TEST_DATA_SIZE) );
		buf = fill_by_random(buf, TEST_DATA_SIZE);
		memset(buf2, '\0', TEST_DATA_SIZE);
		/**/
		/*write more than want read*/
		EXPECT_EQ( TEST_DATA_SIZE, write_sockf(r_sockf, buf, TEST_DATA_SIZE) );
		EXPECT_EQ( TEST_DATA_SIZE/2, read_sockf(w_sockf, buf2, TEST_DATA_SIZE/2) );
		/*write less data than want read*/
		EXPECT_EQ( TEST_DATA_SIZE/2, write_sockf(w_sockf, buf, TEST_DATA_SIZE/2) );
		EXPECT_EQ( TEST_DATA_SIZE/2, read_sockf(r_sockf, buf2, TEST_DATA_SIZE) );
		EXPECT_EQ( 0, strncmp(buf, buf2, TEST_DATA_SIZE/2) );
		buf = fill_by_random(buf, TEST_DATA_SIZE);
		memset(buf2, '\0', TEST_DATA_SIZE);
		/**/
		/*simple use REQ REP patterns */
		EXPECT_EQ( TEST_DATA_SIZE, write_sockf(r_sockf, buf, TEST_DATA_SIZE) );
		EXPECT_EQ( TEST_DATA_SIZE, read_sockf(w_sockf, buf2, TEST_DATA_SIZE) );
		EXPECT_EQ( 0, strncmp(buf, buf2, TEST_DATA_SIZE) );
		EXPECT_EQ( 3, write_sockf(w_sockf, "req", 3) );
		EXPECT_EQ( 3, read_sockf(r_sockf, buf, 3) );
		EXPECT_EQ( 6, write_sockf(r_sockf, "buffer", 6) );
		EXPECT_EQ( 6, read_sockf(w_sockf, buf, 6) );
		/*write more data than trying read, and simulate use sockets like zvm style*/
		buf = fill_by_random(buf, TEST_DATA_SIZE);
		memset(buf2, '\0', TEST_DATA_SIZE);
		uint32_t req_len = 1024;
		uint32_t readed_req_len = 0;
		EXPECT_EQ( (ssize_t)sizeof(req_len), write_sockf(w_sockf, (const char*)&req_len, sizeof(req_len)) ); //send req
		EXPECT_EQ( (ssize_t)sizeof(readed_req_len), read_sockf(r_sockf, (char*)&readed_req_len, sizeof(readed_req_len)) ); //read req
		EXPECT_EQ( readed_req_len, write_sockf(r_sockf, buf, readed_req_len) ); //send req data
		EXPECT_EQ( readed_req_len, read_sockf(w_sockf, buf2, readed_req_len) ); //read req
		EXPECT_EQ( 0, strncmp(buf, buf2, readed_req_len) );
	}
	close_sockf(zpool, w_sockf);
	close_sockf(zpool, r_sockf);
	EXPECT_EQ(ERR_OK, zeromq_term(zpool) );
	free(zpool);
}


TEST_F(ZmqNetwTests, TestSockfIfZmqInitFailed) {
	struct zeromq_pool *zpool = (struct zeromq_pool *) malloc(sizeof(struct zeromq_pool *));
	EXPECT_EQ(ERR_OK, init_zeromq_pool(zpool) );
	/*term zmq context, emulate zmq init failed*/
	zmq_term(zpool->context);
	/*create db_records struct*/
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	db_records.array = (struct db_record_t*)malloc( sizeof(struct db_record_t)*db_records.maxcount );
	memset(db_records.array, '\0', sizeof(struct db_record_t)*db_records.maxcount);
	/*add test item into db_records*/
	struct db_record_t *record1 = &db_records.array[db_records.count++];
	/*set some record1 data*/
	record1->fd = 3;
	record1->fmode = 'w';
	record1->endpoint = (char*)__endpoint1;
	record1->sock = ZMQ_PUSH;
	/*records added*/
	struct sock_file_t* w_sockf = open_sockf( zpool, &db_records, 3);
	/*illustrate that opening is failed*/
	EXPECT_EQ( (struct sock_file_t*)NULL, w_sockf);
	EXPECT_EQ( ERR_BAD_ARG, close_sockf(zpool, w_sockf) );
	EXPECT_EQ(ERR_OK, zeromq_term(zpool) );
	free(zpool);
}


TEST_F(ZmqNetwTests, TestSockfBadArgs) {
	EXPECT_EQ( ERR_BAD_ARG, init_zeromq_pool(NULL) );
	EXPECT_EQ( (struct sock_file_t*)NULL, sockf_by_fd(NULL, 0) );
	struct zeromq_pool* zpool = NULL;
	struct sock_file_t* sockf = NULL;
	struct db_records_t* db_records = NULL;
	EXPECT_EQ( ERR_BAD_ARG, add_sockf_copy_to_array( zpool, sockf) );
	EXPECT_EQ( ERR_BAD_ARG, remove_sockf_from_array_by_fd(zpool, 0) );
	EXPECT_EQ( ERR_BAD_ARG, close_sockf(zpool, sockf) );
	zpool = (struct zeromq_pool*)malloc(sizeof(struct zeromq_pool));
	memset(zpool, '\0', sizeof(struct zeromq_pool));
	db_records = (struct db_records_t*)malloc(sizeof(struct db_records_t));
	memset(db_records, '\0', sizeof(struct db_records_t));
	/*zpool array member is not properly configured*/
	EXPECT_EQ( ERR_BAD_ARG, add_sockf_copy_to_array( zpool, sockf) );
	EXPECT_EQ( ERR_BAD_ARG, remove_sockf_from_array_by_fd( zpool, 0) );
	EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( NULL, NULL, 0) );
	/*test using db_records not yet properly initialized*/
	EXPECT_EQ( (struct sock_file_t*)NULL, open_sockf( zpool, db_records, 0) );
	EXPECT_EQ(-1, write_sockf(NULL, NULL, 1000000) );
	sockf = (struct sock_file_t*)malloc(sizeof(struct sock_file_t));
	memset(sockf, '\0', sizeof(struct sock_file_t) );
	EXPECT_EQ(-1, write_sockf(sockf, "testdata", 8) );
	EXPECT_EQ(-1, write_sockf(sockf, "testdata", -1) );
	EXPECT_EQ(-1, read_sockf(NULL, NULL, 1000000) );
	char *test = (char*)malloc(8);
	EXPECT_EQ(-1, read_sockf(sockf, test, 8) );
	EXPECT_EQ(-1, read_sockf(sockf, test, -1) );
	EXPECT_EQ( ERR_BAD_ARG, init_zeromq_pool(NULL) );
	EXPECT_EQ( ERR_BAD_ARG, zeromq_term(NULL) );
	zpool->sockf_array = (struct sock_file_t*) malloc(sizeof(struct sock_file_t));
	EXPECT_NE( ERR_OK, zeromq_term(zpool) );
}



TEST_F(ZmqNetwTests, TestSockfOpenAllCloseAll) {
	struct zeromq_pool *zpool = (struct zeromq_pool *) malloc(sizeof(struct zeromq_pool *));
	struct db_records_t db_records = {NULL, 0, DB_RECORDS_GRANULARITY, 0};
	EXPECT_EQ(ERR_OK, init_zeromq_pool(zpool) );
	EXPECT_EQ(ERR_OK, get_all_records_from_dbtable(TEST_DB_PATH, "manager\0", &db_records ) );
	/*****test db records retrieved*/
	EXPECT_EQ(ERR_OK, open_all_comm_files(zpool, &db_records) );
	/******Sockets opened and should be exist*/
	for ( int i=0; i < db_records.count; i++ ){
		if ( db_records.array[i].fd > 2 ){
			ASSERT_NE( (struct sock_file_t *)NULL, sockf_by_fd(zpool, db_records.array[i].fd) );
		}
	}
	EXPECT_EQ(ERR_OK, close_all_comm_files(zpool) );
	EXPECT_EQ(ERR_OK, zeromq_term(zpool) );
	free(zpool);
}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	NaClLogSetVerbosity(-10); /*just disable logging attempts*/
	return RUN_ALL_TESTS();
}
