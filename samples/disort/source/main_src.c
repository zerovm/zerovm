/*
 * main_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "api/zvm.h"
#include "defines.h"
#include "sort.h"
#include "dsort.h"
#include "comm_src.h"

#define STDIN 0


int main(int argc, char **argv){
	int nodeid = -1;
	WRITE_FMT_LOG(LOG_DEBUG, "Source node started: argc=%d argv=%p \n", argc, argv);

	for ( int i=0; i < argc; i++ ){
		WRITE_FMT_LOG(LOG_DEBUG, "argv[%d]=%s \n", i, argv[i]);
	}
	if ( argc < 2 ){
		WRITE_LOG(LOG_ERR, "Required 1 arg unique node integer id\n");
		return 1;
	}
	nodeid = atoi(argv[1]);
	WRITE_FMT_LOG(LOG_DEBUG, "nodeid=%d \n", nodeid);

	BigArrayPtr unsorted_array = NULL;
	BigArrayPtr sorted_array = NULL;

	/*get unsorted data, read from stdin*/
	const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;
	unsorted_array = malloc( data_size );
	if ( unsorted_array ){
		/*c99 use of stdin read*/
		const ssize_t readed = read( STDIN, (void*)unsorted_array, data_size);
		WRITE_FMT_LOG(LOG_ERR, "readed input file, expected size=%d, read size=%d\n", data_size, readed);
		assert(readed == data_size );

//		/*zerovm use, non standard*/
//		BigArrayPtr input_data = (BigArrayPtr)setup.channels[InputChannel].buffer;
//		if ( input_data ){
//			WRITE_FMT_LOG(LOG_ERR, "stdin size =%d, data size=%d\n",
//					setup.channels[InputChannel].bsize, data_size);
//			assert( data_size == setup.channels[InputChannel].bsize );
//			memcpy(unsorted_array, input_data, data_size);
//		}
//		else{
//			WRITE_LOG(LOG_ERR, "no stdin\n");
//		}
	}
	WRITE_LOG(LOG_DETAILED_UI, "Source data read complete\n");

	/*sort data locally */
	sorted_array = alloc_sort( unsorted_array, ARRAY_ITEMS_COUNT );
	WRITE_LOG(LOG_DETAILED_UI, "Sorted locally, verify sorting result\n");

	//if first part of sorting in single thread are completed
	if ( test_sort_result( unsorted_array, sorted_array, ARRAY_ITEMS_COUNT ) ){
		if ( ARRAY_ITEMS_COUNT ){
			WRITE_FMT_LOG(LOG_UI, "Single process sorting complete min=%u, max=%u: TEST OK.\n",
					sorted_array[0], sorted_array[ARRAY_ITEMS_COUNT-1] );
		}

		/*send crc of sorted array to the manager node*/
		uint32_t crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
		WRITE_FMT_LOG(LOG_DEBUG, "write crc into fd=%d", SOURCE_FD_WRITE_CRC);
		write_crc( SOURCE_FD_WRITE_CRC, crc );
		/*send of crc complete*/

		/*prepare histogram data, with step defined by BASE_HISTOGRAM_STEP*/
		int histogram_len = 0;
		HistogramArrayPtr histogram_array = alloc_histogram_array_get_len(
				sorted_array, 0, ARRAY_ITEMS_COUNT, BASE_HISTOGRAM_STEP, &histogram_len );

		struct Histogram single_histogram;
		single_histogram.src_nodeid = nodeid;
		single_histogram.array_len = histogram_len;
		single_histogram.array = histogram_array;

		/*send histogram to manager*/
		write_histogram( SOURCE_FD_WRITE_HISTOGRAM, &single_histogram );
		/*read request for detailed histogram and send it to manager*/
		read_requests_write_detailed_histograms( SOURCE_FD_READ_D_HISTOGRAM_REQ, SOURCE_FD_WRITE_D_HISTOGRAM_REQ,
				nodeid, sorted_array, ARRAY_ITEMS_COUNT );
		WRITE_LOG(LOG_UI, "\n!!!!!!!Histograms Sending complete!!!!!!.\n");

		/*read range request (data start, end, dest node id) from manager node*/
		struct request_data_t req_data_array[SRC_NODES_COUNT];
		init_request_data_array( req_data_array, SRC_NODES_COUNT);
		read_range_request( SOURCE_FD_READ_SEQUENCES_REQ, req_data_array );

		/*send array data to the destination nodes, bounds for pieces of data was
		 * received previously with range request */
		for ( int i=0; i < SRC_NODES_COUNT; i++ ){
			int dst_nodeid = req_data_array[i].dst_nodeid;
			int dst_write_fd = dst_nodeid - FIRST_DEST_NODEID + SOURCE_FD_WRITE_SORTED_RANGES_REQ;
			int dst_read_fd = dst_nodeid - FIRST_DEST_NODEID + SOURCE_FD_READ_SORTED_RANGES_REQ;
			WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges write req fd=%d, read req fd=%d", dst_write_fd, dst_read_fd );
			WRITE_FMT_LOG(LOG_DEBUG, "req_data_array[i].dst_nodeid=%d", req_data_array[i].dst_nodeid );
			write_sorted_ranges( dst_write_fd, dst_read_fd, &req_data_array[i], sorted_array );
		}
		WRITE_LOG(LOG_UI, "Sending Ranges Complete-OK");

		free(unsorted_array);
		free(sorted_array);
	}
	else{
		WRITE_LOG(LOG_UI, "Single process sorting failed: TEST FAILED.\n");
		return 1;
	}
	return 0;
}

