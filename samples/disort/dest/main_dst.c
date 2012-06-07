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
#include "comm_dst.h"


int main(int argc, char **argv){
	int nodeid = -1;
	if ( argc < 2 ){
		return 1;
	}
	nodeid = atoi(argv[1]);
	WRITE_FMT_LOG(LOG_UI, "[%d] Destination node started\n", nodeid);

	BigArrayPtr unsorted_array = NULL;
	BigArrayPtr sorted_array = NULL;

	/*receive sorted ranges from all source nodes*/
	size_t array_size = ARRAY_ITEMS_COUNT*sizeof(BigArrayItem);
	unsorted_array = malloc( array_size );
	memset(unsorted_array, '\0', array_size);
	repreq_read_sorted_ranges( DEST_FD_READ_SORTED_RANGES_START, nodeid, unsorted_array,
			ARRAY_ITEMS_COUNT, SRC_NODES_COUNT );

#ifdef MERGE_ON_FLY
	sorted_array = unsorted_array;
#else
	/*local sort of received pieces*/
	sorted_array = alloc_sort( unsorted_array, ARRAY_ITEMS_COUNT );
#endif

	/*save sorted array to output file*/
	const size_t data_size = sizeof(BigArrayItem)*ARRAY_ITEMS_COUNT;
	if ( sorted_array ){
		const ssize_t wrote = write( 1, sorted_array, data_size);
		assert(wrote == data_size );
	}

	write_sort_result( DEST_FD_WRITE_SORT_RESULT, nodeid, sorted_array, ARRAY_ITEMS_COUNT );

	free(unsorted_array);
	WRITE_LOG( LOG_UI,  "Destination node complete\n");
	return 0;
}

