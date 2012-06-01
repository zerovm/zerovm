/*
 * Tests for db configuration read
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>

#include "gtest/gtest.h"
#include "src/platform/nacl_log.h"
#include "src/networking/errcodes.h"
extern "C" {
#include "src/networking/sqluse_srv.h"
}

#define TEST_NODENAME "test\0"
#define TEST_ENDPOINT "ipc://test\0"
#define TEST_FPATH "/in/file\0"
#define TEST_FD "3"
#define TEST_MODE_W "w\0"

#define SERVER_DB_PATH "gtest/data/zerovm_test.db"

// Test harness for routines in sqluse_srv.c
class SqlUseSrvTests : public ::testing::Test {
public:
	SqlUseSrvTests(){}
};


static int
records_comparator( const void *m1, const void *m2 )
{
	const struct db_record_t *t1= (const struct db_record_t* )(m1);
	const struct db_record_t *t2= (const struct db_record_t* )(m2);

	if ( t1->fd < t2->fd )
		return -1;
	else if ( t1->fd > t2->fd )
		return 1;
	else return 0;
	return 0;
}



TEST_F(SqlUseSrvTests, TestReadDbItems) {
	struct db_records_t db_records;
	memset(&db_records, '\0', sizeof(struct db_records_t));
	EXPECT_EQ( ERR_OK, get_all_records_from_dbtable(SERVER_DB_PATH, "manager", &db_records) );
	/*sort db records to verify that file descriptors are unique*/
	qsort( db_records.array, db_records.count, sizeof(struct db_record_t), records_comparator );
	/*test sorted records, should not found duplicate file descriptors*/
	for (int i=0; i < db_records.count-1; i++){
		EXPECT_LT( db_records.array[i].fd, db_records.array[i+1].fd );
	}

	EXPECT_NE(0, db_records.count);
	struct db_record_t *record = match_db_record_by_fd(&db_records, 3);
	if ( record ){
		EXPECT_EQ(3, record->fd);
	}
	EXPECT_EQ( (struct db_record_t*)NULL, match_db_record_by_fd(&db_records, -1) );
}

TEST_F(SqlUseSrvTests, TestReadDbBadArgs) {
	struct db_records_t db_records;
	EXPECT_EQ( ERR_BAD_ARG, get_all_records_from_dbtable( NULL, NULL, NULL) );
	EXPECT_EQ( ERR_BAD_ARG, get_all_records_from_dbtable( SERVER_DB_PATH, "manager", NULL) );
	EXPECT_EQ( ERR_BAD_ARG, get_all_records_from_dbtable( SERVER_DB_PATH, NULL, &db_records) );
}

TEST_F(SqlUseSrvTests, TestDbItemsParser) {
	struct db_records_t db_records;
	db_records.maxcount = DB_RECORDS_GRANULARITY;
	db_records.array = (struct db_record_t*) malloc( sizeof(struct db_record_t)*db_records.maxcount );
	db_records.count = 0;
	char **row_values_array = (char**)malloc(sizeof(char*)*ECOL_COLUMNS_COUNT);
	row_values_array[ECOL_NODENAME] = (char*)TEST_NODENAME;
	row_values_array[ECOL_FTYPE] = (char*)FILE_TYPE_MSQ;
	row_values_array[ECOL_SOCK] = (char*)PUSH;
	row_values_array[ECOL_METHOD] = (char*)METHOD_CONNECT;
	row_values_array[ECOL_ENDPOINT] = (char*)TEST_ENDPOINT;
	row_values_array[ECOL_FMODE] = (char*)TEST_MODE_W;
	row_values_array[ECOL_FPATH] = (char*)TEST_FPATH;
	row_values_array[ECOL_FD] = (char*)TEST_FD;
	get_dbrecords_callback( &db_records, ECOL_COLUMNS_COUNT, row_values_array, NULL);
	ASSERT_TRUE( 1==db_records.count );
}


int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	NaClLogSetVerbosity(-10); /*just disable logging attempts*/
	return RUN_ALL_TESTS();
}
