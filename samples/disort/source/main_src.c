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

#include "api/zrt.h"
#include "defines.h"
#include "sort.h"
#include "dsort.h"
#include "comm_src.h"
#include "cpuid.h"
#include "bitonic_sort.h"

#define STDIN 0

static int
quicksort_reqdata_by_destnodeid_comparator( const void *m1, const void *m2 ){

	const struct request_data_t *t1= (const struct request_data_t*)(m1);
	const struct request_data_t *t2= (const struct request_data_t*)(m2);

	if ( t1->dst_nodeid < t2->dst_nodeid )
		return -1;
	else if ( t1->dst_nodeid > t2->dst_nodeid )
		return 1;
	else return 0;
	return 0;
}

/* need for SSE aligned memory access */
void *aligned_malloc(size_t size, size_t align)
{
	void *mem = malloc(size + align + sizeof(void*));
	void **ptr = (void**)(((size_t)mem + align + sizeof(void*)) & ~(align - 1));
	ptr[-1] = mem;
	return ptr;
}

/* replaces free() */
void aligned_free(void *ptr)
{
	free(((void**)ptr)[-1]);
}


int main(int argc, char **argv){
	int nodeid = -1;
	WRITE_FMT_LOG(LOG_DEBUG, "Source node started: argc=%d argv=%p \n", argc, argv);

	for ( int i=0; i < argc; i++ ){
		WRITE_FMT_LOG(LOG_DEBUG, "argv[%d]=%s \n", i, argv[i]);
	}
	if ( argc < 2 ){
		WRITE_LOG(LOG_ERR, "Required 1 arg unique node integer id\n");
		abort();
	}
	nodeid = atoi(argv[1]);
	WRITE_FMT_LOG(LOG_DEBUG, "nodeid=%d \n", nodeid);

	BigArrayPtr sorted_array = NULL;
	const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;

	/*If can use bitonic sort*/
	if ( test_sse41_CPU() ){
		WRITE_LOG(LOG_ERR, "allocate bitonic sort memories\n");
		/*we needed an extra buf to use with bitonic sort*/
		BigArrayPtr extra_buf = NULL;

		/*allocated memory should be aligned*/
		extra_buf = aligned_malloc(data_size, SSE2_ALIGNMENT);
		if ( !extra_buf ) {
			WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
			abort();
		}
		sorted_array = aligned_malloc(data_size, SSE2_ALIGNMENT);
		if (!sorted_array) {
			WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
			abort();
		}
		/*Read source data from STDIN*/
		const ssize_t readed = read( STDIN, (void*)sorted_array, data_size);
		WRITE_FMT_LOG(LOG_ERR, "readed input file, expected size=%d, read size=%d\n", data_size, readed);
		assert(readed == data_size );
		WRITE_LOG(LOG_DETAILED_UI, "Start bitonic sorting\n");
		bitonic_sort_chunked((float*)sorted_array, ARRAY_ITEMS_COUNT, (float*)extra_buf, DEFAULT_CHUNK_SIZE);
		WRITE_LOG(LOG_DETAILED_UI, "Bitonic sorting complete\n");
		aligned_free(extra_buf);
	}
	/*If can't use bitonic  - then use c qsort*/
	else{
		WRITE_LOG(LOG_ERR, "qsort will used\n");
		BigArrayPtr unsorted_array = NULL;
		unsorted_array = malloc( data_size );
		if ( !unsorted_array ) {
			WRITE_LOG(LOG_ERR, "Can't allocate memories\n");
			abort();
		}
		if ( unsorted_array ){
			/*Read source data from STDIN*/
			const ssize_t readed = read( STDIN, (void*)unsorted_array, data_size);
			WRITE_FMT_LOG(LOG_ERR, "readed input file, expected size=%d, read size=%d\n", data_size, readed);
			assert(readed == data_size );
		}
		WRITE_LOG(LOG_DETAILED_UI, "Start qsort sorting\n");
		sorted_array = alloc_copy_array( unsorted_array, ARRAY_ITEMS_COUNT );
		qsort( sorted_array, ARRAY_ITEMS_COUNT, sizeof(BigArrayItem), quicksort_BigArrayItem_comparator );
		free(unsorted_array);
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
	read_requests_write_detailed_histograms( SOURCE_FD_READ_D_HISTOGRAM_REQ, SOURCE_FD_WRITE_D_HISTOGRAM_REP,
			nodeid, sorted_array, ARRAY_ITEMS_COUNT );
	WRITE_LOG(LOG_UI, "\n!!!!!!!Histograms Sending complete!!!!!!.\n");

	/*read range request (data start, end, dest node id) from manager node*/
	struct request_data_t req_data_array[SRC_NODES_COUNT];
	init_request_data_array( req_data_array, SRC_NODES_COUNT);
	read_range_request( SOURCE_FD_READ_SEQUENCES_REQ, req_data_array );

	/*sort request data by dest node id to be deterministic*/
	qsort( req_data_array, SRC_NODES_COUNT, sizeof(struct request_data_t), quicksort_reqdata_by_destnodeid_comparator );

	/*send array data to the destination nodes, bounds for pieces of data was
	 * received previously with range request */
	for ( int i=0; i < SRC_NODES_COUNT; i++ ){
		int dst_nodeid = req_data_array[i].dst_nodeid;
		int dst_write_fd = dst_nodeid - FIRST_DEST_NODEID + SOURCE_FD_WRITE_SORTED_RANGES_START;
		WRITE_FMT_LOG(LOG_DEBUG, "write_sorted_ranges write fd=%d", dst_write_fd );
		WRITE_FMT_LOG(LOG_DEBUG, "req_data_array[i].dst_nodeid=%d", req_data_array[i].dst_nodeid );
		write_sorted_ranges( dst_write_fd, &req_data_array[i], sorted_array );
	}
	WRITE_LOG(LOG_UI, "Sending Ranges Complete-OK");

	if ( test_sse41_CPU() )
		aligned_free(sorted_array);
	else
		free(sorted_array);

	return 0;
}

