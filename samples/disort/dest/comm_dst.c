/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm_dst.h"
#include "dsort.h"
#include "defines.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>



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
repreq_read_sorted_ranges( int fdr, int fdw, int nodeid,
		BigArrayPtr dst_array, int dst_array_len, int ranges_count ){
	char reply='-';

	WRITE_FMT_LOG(LOG_DEBUG, "[%d] Read ranges from fd=%d\n", nodeid, fdr);
	int recv_bytes_count = 0;
	for (int i=0; i < ranges_count; i++)
	{
		struct packet_data_t t;
		read( fdr, (char*)&t, sizeof(t) );
		if (EPACKET_RANGE != t.type ){
			WRITE_FMT_LOG(LOG_DEBUG, "assert t.type=%d %d(EPACKET_RANGE)", t.type, EPACKET_RANGE);
			assert(0);
		}
		write(fdw, &reply, 1);
		read( fdr, (char*)&dst_array[recv_bytes_count/sizeof(BigArrayItem)], t.size );
		recv_bytes_count += t.size;
		write(fdw, &reply, 1);
	}
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] channel_receive_sorted_ranges OK\n", nodeid );
}

/*writing data:
 * 1x struct sort_result*/
void
write_sort_result( int fdw, int nodeid, BigArrayPtr sorted_array, int len ){
	if ( !len ) return;
	uint32_t sorted_crc = array_crc( sorted_array, ARRAY_ITEMS_COUNT );
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] send_sort_result: min=%d, max=%d, crc=%u",
			nodeid, sorted_array[0], sorted_array[len-1], sorted_crc );
	struct sort_result result;
	result.nodeid = nodeid;
	result.min = sorted_array[0];
	result.max = sorted_array[len-1];
	result.crc = sorted_crc;
	write(fdw, &result, sizeof(result));
}


