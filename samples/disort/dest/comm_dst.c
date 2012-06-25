/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm.h"
#include "comm_dst.h"
#include "dsort.h"
#include "defines.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>



/*reading data
 * 1x struct packet_data_t (packet type, size of array)
 *writing data
 * 1x char, reply
 *reading data
 * packet_data_t::size x BigArrayItem (sorted part of array)
 *writing data
 * 1x char, reply
 * */
void
repreq_read_sorted_ranges( int fdr, int nodeid,
		BigArrayPtr dst_array, int dst_array_len, int ranges_count ){
#ifdef MERGE_ON_FLY
	BigArrayPtr merge_array = calloc(sizeof(BigArrayItem), dst_array_len);
#endif
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] Read ranges from fd=%d\n", nodeid, fdr);
	int recv_bytes_count = 0;
	for (int i=0; i < ranges_count; i++)
	{
		int current_read_fd = fdr+i;
		WRITE_FMT_LOG(LOG_DEBUG, "repreq_read_sorted_ranges #%d\n", i);
		struct packet_data_t t;
		read_channel( current_read_fd, (char*)&t, sizeof(t) );
		if (EPACKET_RANGE != t.type ){
			WRITE_FMT_LOG(LOG_DEBUG, "assert t.type=%d %d(EPACKET_RANGE)\n", t.type, EPACKET_RANGE);
			assert(0);
		}
#ifndef MERGE_ON_FLY
		read_channel( current_read_fd, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], t.size );
#endif
#ifdef MERGE_ON_FLY
		/* 1. Copy to merge array first chunk
		 * 2. Copy to merge array next chunk, starting from next item from first chunk
		 * 3. Merge both chunks and write merged result into dst_array
		 * */
		int count_items_part1 = t.size/sizeof(BigArrayItem);
		int count_items_part2 = recv_bytes_count/sizeof(BigArrayItem);

		read_channel( current_read_fd, (char*)&merge_array[count_items_part2], t.size );
		/*if both chunks recevied*/
		if ( count_items_part2 != 0 ){
			/*merge both chunks*/
			dst_array = merge( dst_array, merge_array, count_items_part2,
					&merge_array[count_items_part2], count_items_part1 );
		}
		else{
			strncpy((char*)dst_array, (const char*)merge_array, t.size );
		}
#endif
		recv_bytes_count += t.size;
	}
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] channel_receive_sorted_ranges OK\n", nodeid );
}

/*writing data:
 * 1x struct sort_result*/
void
write_sort_result( int fdw, int nodeid, BigArrayPtr sorted_array, int len ){
	if ( !len ) return;
	uint32_t sorted_crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] send_sort_result: min=%u, max=%u, crc=%u\n",
			nodeid, sorted_array[0], sorted_array[len-1], sorted_crc );
	struct sort_result result;
	result.nodeid = nodeid;
	result.min = sorted_array[0];
	result.max = sorted_array[len-1];
	result.crc = sorted_crc;
	write_channel(fdw, (const char*)&result, sizeof(result));
}


