/*
 * manager.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "api/zvm.h"
#include "defines.h"
#include "comm_man.h"
#include "distr_common.h"
#include "histanlz.h"
#include "dsort.h"


int main(int argc, char **argv){
	int nodeid = -1;
	WRITE_FMT_LOG(LOG_DEBUG, "Manager node started: argc=%d argv=%p \n", argc, argv);
	for ( int i=0; i < argc; i++ ){
		WRITE_FMT_LOG(LOG_DEBUG, "argv[%d]=%s \n", i, argv[i]);
	}
	if ( argc < 2 ){
		WRITE_LOG(LOG_ERR, "Required 1 arg unique node integer id\n");
		return 1;
	}
	nodeid = atoi(argv[1]);
	WRITE_FMT_LOG(LOG_DEBUG, "Manager node started: nodeid=%d \n", nodeid);
	WRITE_LOG(LOG_DEBUG, "Manager node started");
	exit(0);

	/*crc reading from all source nodes*/
	uint32_t crc_array[SRC_NODES_COUNT];
	memset(crc_array, '\0', SRC_NODES_COUNT*sizeof(uint32_t));
	read_crcs(MANAGER_FD_READ_CRC, crc_array);
	/*crc read ok*/

	/******** Histograms reading*/
	struct Histogram histograms[SRC_NODES_COUNT];
	WRITE_LOG(LOG_DEBUG, "Recv histograms");
	recv_histograms( MANAGER_FD_READ_HISTOGRAM, (struct Histogram*) &histograms, SRC_NODES_COUNT );
	WRITE_LOG(LOG_DEBUG, "Recv histograms OK");
	/******** Histograms reading OK*/

	/*Sort histograms by src_nodeid, because recv order is unexpected,
	 * but histogram analizer algorithm is required deterministic order*/
	qsort( histograms, SRC_NODES_COUNT, sizeof(struct Histogram), histogram_srcid_comparator );

	WRITE_LOG(LOG_DEBUG, "Analize histograms and request detailed histograms");
	struct request_data_t** range = alloc_range_request_analize_histograms(
			ARRAY_ITEMS_COUNT, nodeid, histograms, SRC_NODES_COUNT );
	WRITE_LOG(LOG_DEBUG, "Analize histograms and request detailed histograms OK");

	for (int i=0; i < SRC_NODES_COUNT; i++ )
	{
		WRITE_FMT_LOG(LOG_DEBUG, "SOURCE PART N %d:\n", i );
		print_request_data_array( range[i], SRC_NODES_COUNT );
	}

	/// sending range requests to every source node
	for (int i=0; i < SRC_NODES_COUNT; i++ ){
		///
		int src_write_fd = range[0][i].src_nodeid - FIRST_SOURCE_NODEID + MANAGER_FD_WRITE_RANGE_REQUEST;
		WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges fdw=%d", src_write_fd );
		write_range_request( src_write_fd, range, SRC_NODES_COUNT, i );
	}

	for ( int i=0; i < SRC_NODES_COUNT; i++ ){
		free(histograms[i].array);
		free( range[i] );
	}
	free(range);

	struct sort_result *results = read_sort_result( MANAGER_FD_READ_SORT_RESULTS, SRC_NODES_COUNT );
	/*sort results by nodeid, because receive order not deterministic*/
	qsort( results, SRC_NODES_COUNT, sizeof(struct sort_result), sortresult_comparator );
	int sort_ok = 1;
	uint32_t crc_test_unsorted = 0;
	uint32_t crc_test_sorted = 0;
	for ( int i=0; i < SRC_NODES_COUNT; i++ ){
		crc_test_unsorted = (crc_test_unsorted+crc_array[i]% CRC_ATOM) % CRC_ATOM;
		crc_test_sorted = (crc_test_sorted+results[i].crc% CRC_ATOM) % CRC_ATOM;
		if ( i>0 ){
			if ( !(results[i].max > results[i].min && results[i-1].max < results[i].min) )
				sort_ok = 0;
		}
		WRITE_FMT_LOG(LOG_DEBUG, "results[%d], nodeid=%d, min=%u, max=%u\n", i, results[i].nodeid, results[i].min, results[i].max);
		fflush(0);
	}

	WRITE_FMT_LOG(LOG_UI, "Distributed sort complete, Test %d, %d, %d\n", sort_ok, crc_test_unsorted, crc_test_sorted );
	if ( crc_test_unsorted == crc_test_sorted ){
		WRITE_LOG(LOG_UI, "crc OK");
	}
	else{
		WRITE_LOG(LOG_UI,"crc FAILED");
	}
	return 0;
}
