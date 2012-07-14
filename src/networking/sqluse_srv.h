/*
 * sqluse_srv.h
 *
 *  Created on: 02.05.2012
 *      Author: YaroslavLitvinov
 * This is channels configuration reading engine used by server side;
 * All DB data represented by strings, and can be copied into data structure unchanged;
 * But endpoint column string data can be used as format string for sprintf if %d format parameter is found,
 * and db_records_t::cid as format value, contains properly value setted up before request;
 * Some columns should converted into internal representation as enum or integer;
 * The main selective parameter is nodename, all records contains same nodename are related to same node.
 */

#ifndef NETWORKING_SQLUSE_CLI_H_
#define NETWORKING_SQLUSE_CLI_H_

struct db_records_t
{
  struct db_record_t *array;
  int count;
  int maxcount;
  int cid; /*it's contains unique client id and used to post processing db data*/
};

#define FILE_TYPE_STD "std\0"
#define FILE_TYPE_MSQ "msq\0"

#define REQREP  "REQREP\0"

enum
{
  ECOL_NODENAME = 0, ECOL_ENDPOINT, ECOL_FMODE, ECOL_FD, ECOL_COLUMNS_COUNT
};

struct db_record_t
{
  char *nodename;
  int sock;
  char *endpoint;
  char fmode;
  int fd;
};

/* saves testid, it say to stub impementation which test case need emulate.
 * It should be used only from test framework and only sqluse_srv_stub.c has implementation, sqluse_srv.c has empty implem*/
void tune_sqlstub_for_test(int testid);

enum
{
  DB_RECORDS_GRANULARITY = 20
};
/*search record by file descriptor*/
struct db_record_t* match_db_record_by_fd(struct db_records_t *records, int fd);

/* In use case it's should not be directly called, but only as callback for sqlite API;*/
int get_dbrecords_callback(void *file_records, int argc, char **argv, char **azColName);

/*Issue db request.
 * @param path DB filename
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer
 * @return LOAD_SQL_STATEMENT_EXEC_ERROR if sql statement execute error */
int get_all_records_from_dbtable(const char *path, const char *nodename,
    struct db_records_t *db_records);

#endif /* NETWORKING_SQLUSE_CLI_H_ */
