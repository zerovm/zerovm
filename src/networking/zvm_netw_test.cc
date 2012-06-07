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
#include "src/networking/zvm_netw.h"
#include "src/networking/zmq_netw.h"
}

const char *__endpoint1 = "ipc:///tmp/test1\0";
#define TEST_DATA_SIZE 1000000
#define TEST_DB_PATH "gtest/data/zerovm_test.db"

// Test harness for routines in zmq_netw.c
class ZvmNetwTests : public ::testing::Test {
public:
	ZvmNetwTests(){}
};



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


ssize_t test_read(int fd, char* buf, size_t count){
	/*write request*/
	/*read data*/
	return 0;
}


TEST_F(ZvmNetwTests, TestInitZvmNetworking) {
	/*load from DB sockets data by key=test*/
	char buf[]= {"tset\0"};
	EXPECT_EQ(EZVM_OK, init_zvm_networking(TEST_DB_PATH, "test", 1) );
	const int fdw = 3;
	const int fdr = 4;
	int capwrite = capabilities_for_file_fd(fdw);
	int capread = capabilities_for_file_fd(fdr);
	EXPECT_EQ( 1, EWRITE == capwrite );
	EXPECT_EQ( 1, EREAD == capread );
	EXPECT_EQ( -1, commf_write(fdr, "test", 4) );
	EXPECT_EQ( -1, commf_read(fdw, buf, 4) );
	EXPECT_EQ(EZVM_OK, term_zvm_networking() );
}


TEST_F(ZvmNetwTests, TestZvmWrite1) {
	/*load from DB sockets data by key=test*/
	EXPECT_EQ(EZVM_OK, init_zvm_networking(TEST_DB_PATH, "test", 1) );
	const int fdw = 3;
	const int fdr = 4;
	const int testlen = 100;
	char *buf_w = alloc_fill_random(testlen);
	char *buf_r = (char*)malloc(testlen);
	memset(buf_r, '\0', testlen);

	/*Use zmq_netw. we do need this to test communication inside one application
	 *User should not use zmq_netw directly, only zvm_netw::commf_read, commf_write to read and write sockets */
	struct sock_file_t* sockf_w = sockf_by_fd(zpool_forunittest(), fdw);
	EXPECT_NE( (struct sock_file_t*)NULL, sockf_w );
	struct sock_file_t* sockf_r = sockf_by_fd(zpool_forunittest(), fdr);
	EXPECT_NE( (struct sock_file_t*)NULL, sockf_r );

	/*zmq write, simulate request from fdr*/
	struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
	header.req_len = 30;
	EXPECT_EQ( (ssize_t)sizeof(header), write_sockf(sockf_r, (char*)&header, sizeof(header)) );
	/*zvm write*/
	EXPECT_EQ( header.req_len, commf_write(fdw, buf_w, testlen) );
	/*zmq read*/
	EXPECT_EQ( header.req_len, read_sockf(sockf_r, (char*)buf_r, testlen) );

	free(buf_w);
	free(buf_r);
	EXPECT_EQ(EZVM_OK, term_zvm_networking() );
}


//TEST_F(ZvmNetwTests, TestZvmWrite2) {
//	/*load from DB sockets data by key=test*/
//	EXPECT_EQ(EZVM_OK, init_zvm_networking(TEST_DB_PATH, "test", 1) );
//	const int fdw = 3;
//	const int fdr = 4;
//	const int testlen = 100;
//	char *buf_w = alloc_fill_random(testlen);
//	char *buf_r = (char*)malloc(testlen);
//	memset(buf_r, '\0', testlen);
//
//	/*Use zmq_netw. we do need this to test communication inside one application
//	 *User should not use zmq_netw directly, only zvm_netw::commf_read, commf_write to read and write sockets */
//	struct sock_file_t* sockf_w = sockf_by_fd(zpool_forunittest(), fdw);
//	EXPECT_NE( (struct sock_file_t*)NULL, sockf_w );
//	struct sock_file_t* sockf_r = sockf_by_fd(zpool_forunittest(), fdr);
//	EXPECT_NE( (struct sock_file_t*)NULL, sockf_r );
//
//	/*zmq write, simulate request from fdr*/
//	struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
//	header.req_len = 30;
//	EXPECT_EQ( (ssize_t)sizeof(header), write_sockf(sockf_r, (char*)&header, sizeof(header)) );
//	/*zvm write*/
//	EXPECT_EQ( header.req_len, commf_write(fdw, buf_w, header.req_len) );
//	/*zmq read*/
//	EXPECT_EQ( header.req_len, read_sockf(sockf_r, (char*)buf_r, testlen) );
//
//	free(buf_w);
//	free(buf_r);
//	EXPECT_EQ(EZVM_OK, term_zvm_networking() );
//}

//TEST_F(ZvmNetwTests, TestZvmRead) {
//	/*load from DB sockets data by key=test*/
//	EXPECT_EQ(EZVM_OK, init_zvm_networking(TEST_DB_PATH, "test", 1) );
//	const int fdw = 3;
//	const int fdr = 4;
//	const int testlen = 100;
//	char *buf_w = alloc_fill_random(testlen);
//	char *buf_r = (char*)malloc(testlen);
//	memset(buf_r, '\0', testlen);
//
//	/*Use zmq_netw. we do need this to test communication inside one application
//	 *User should not use zmq_netw directly, only zvm_netw::commf_read, commf_write to read and write sockets */
//	struct sock_file_t* sockf_w = sockf_by_fd(zpool_forunittest(), fdw);
//	EXPECT_NE( (struct sock_file_t*)NULL, sockf_w );
//	struct sock_file_t* sockf_r = sockf_by_fd(zpool_forunittest(), fdr);
//	EXPECT_NE( (struct sock_file_t*)NULL, sockf_r );
//
//	//write data for commf_read
//	EXPECT_EQ( testlen-1, write_sockf(sockf_w, buf_w, testlen-1) );
//	/*zvm read: request testlen data, but read only testlen-1, it was actually wrote by sender*/
//	EXPECT_EQ( testlen-1, commf_read(fdr, buf_r, testlen) );
//	/*read request*/
//	struct zvm_netw_header_t header = DEFAULT_ZVM_NETW_HEAD;
//	EXPECT_EQ( (ssize_t)sizeof(header), read_sockf(sockf_w, (char*)&header, sizeof(header)) );
//
//	free(buf_w);
//	free(buf_r);
//	EXPECT_EQ(EZVM_OK, term_zvm_networking() );
//}

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	NaClLogSetVerbosity(-10); /*just disable logging attempts*/
	return RUN_ALL_TESTS();
}
