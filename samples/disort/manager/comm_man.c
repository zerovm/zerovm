/*
 * communic.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */


#include "distr_common.h"
#include "defines.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/*reading crc from every source node, number of src nodes is defined by SRC_NODES_COUNT
 *READ 1x uint32_t, crc*/
void
read_crcs(int fdr, uint32_t *crc_array){
	WRITE_FMT_LOG(LOG_DEBUG, "Reading read_crcs %d count\n", SRC_NODES_COUNT);
	int bytes=0;
	for( int i=0; i < SRC_NODES_COUNT; i++ ){
		WRITE_FMT_LOG(LOG_DEBUG, "Reading crc #%d\n", i);
		bytes=read(fdr, (char*) &crc_array[i], sizeof(uint32_t) );
		WRITE_FMT_LOG(LOG_DEBUG, "%d bytes readed crc=%u\n", bytes, crc_array[i]);
	}
	WRITE_LOG(LOG_DEBUG, "crc read ok");
}


/*READ 1x struct packet_data_t
 *READ array_len x  HistogramArrayItem, array*/
void
recv_histograms( int fdr, struct Histogram *histograms, int wait_histograms_count ){
	WRITE_FMT_LOG(LOG_DEBUG, "Reading histograms %d count\n", wait_histograms_count);
	int bytes;
	for( int i=0; i < wait_histograms_count; i++ ){
		WRITE_FMT_LOG(LOG_DEBUG,"Reading histogram #%d\n", i);
		struct packet_data_t t; t.type = EPACKET_UNKNOWN;
		bytes=read(fdr, (char*) &t, sizeof(t) );
		WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d\n", bytes);
		WRITE_FMT_LOG(LOG_DEBUG,"readed packet: type=%d, size=%d, src_nodeid=%d\n", t.type, (int)t.size, t.src_nodeid );
		struct Histogram *histogram = &histograms[i];
		if ( EPACKET_HISTOGRAM == t.type ){
			histogram->array = malloc( t.size );
			bytes=read(fdr, (char*) histogram->array, t.size);
			WRITE_FMT_LOG(LOG_DEBUG,"r histogram->array bytes=%d\n", bytes);
			histogram->array_len = t.size / sizeof(HistogramArrayItem);
			histogram->src_nodeid = t.src_nodeid;
		}
		else{
			WRITE_FMT_LOG(LOG_ERR, "recv_histograms::wrong packet type %d size %d\n", t.type, (int)t.size);
			exit(-1);
		}
	}
	WRITE_LOG(LOG_DEBUG, "Hitograms ok");
}


/*i/o 2 synchronous files
 *WRITE  1x struct request_data_t
 *READ 1x char, reply
 *WRITE  1x char is_complete flag, if 0
 *READ 1x int, nodeid
 *WRITE  1x char, reply
 *READ 1x size_t, array_len
 *WRITE  1x char, reply
 *READ array_size, array HistogramArrayItem
 * @param complete Flag 0 say to client in request that would be requested again, 1-last request send
 *return Histogram Caller is responsive to free memory after using result*/
struct Histogram*
reqrep_detailed_histograms_alloc( int fdw, int fdr, int nodeid,
		const struct request_data_t* request_data, int complete ){
	char reply;
	WRITE_FMT_LOG(LOG_DEBUG, "[%d] complete=%d, Write detailed histogram requests to %d\n", nodeid, complete, fdw );
	int bytes;
	//send detailed histogram request
	bytes=write(fdw, request_data, sizeof(struct request_data_t));
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d\n", bytes);
	bytes=read(fdr, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d\n", bytes);
	bytes=write(fdw, &complete, sizeof(complete));
	WRITE_FMT_LOG(LOG_DEBUG, "w is_complete bytes=%d\n", bytes);
	WRITE_LOG(LOG_DEBUG, "detailed histograms receiving");
	//recv reply
	struct Histogram *item = malloc(sizeof(struct Histogram));
	bytes=read(fdr, (char*) &item->src_nodeid, sizeof(item->src_nodeid));
	WRITE_FMT_LOG(LOG_DEBUG, "r item->src_nodeid bytes=%d\n", bytes);
	bytes=write(fdw, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d\n", bytes);
	bytes=read(fdr, (char*) &item->array_len, sizeof(item->array_len));
	WRITE_FMT_LOG(LOG_DEBUG, "r item-?array_len bytes=%d\n", bytes);
	bytes=write(fdw, &reply, 1);
	WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d\n", bytes);
	size_t array_size = sizeof(HistogramArrayItem)*item->array_len;
	item->array = malloc(array_size);
	bytes=read(fdr, (char*) item->array, array_size);
	WRITE_FMT_LOG(LOG_DEBUG, "r item->array bytes=%d\n", bytes);

	WRITE_FMT_LOG(LOG_DEBUG, "detailed histograms received from%d: expected len:%d\n",
			item->src_nodeid, (int)(sizeof(HistogramArrayItem)*item->array_len) );
	return item;
}

/*WRITE 1x struct packet_data_t
 *WRITE 1x struct request_data_t*/
void
write_range_request( int fdw, struct request_data_t** range, int len, int i ){
	struct packet_data_t t;
	t.type = EPACKET_SEQUENCE_REQUEST;
	t.src_nodeid = (int)range[i][0].dst_nodeid;
	t.size = len;
	int bytes;
	WRITE_FMT_LOG(LOG_DEBUG, "t.ize=%d, t.type=%d(EPACKET_SEQUENCE_REQUEST)\n", (int)t.size, t.type);
	bytes=write(fdw, &t, sizeof(t));
	WRITE_FMT_LOG(LOG_DEBUG, "w packet_data_t bytes=%d\n", bytes);
	for ( int j=0; j < len; j++ ){
		/*request data copy*/
		struct request_data_t data = range[j][i];
		data.dst_nodeid = j+FIRST_DEST_NODEID;
		bytes=write(fdw, &data, sizeof(struct request_data_t));
		WRITE_FMT_LOG(LOG_DEBUG, "w request_data_t bytes=%d\n", bytes);
		WRITE_FMT_LOG(LOG_DEBUG, "write_range_request %d %d %d %d\n",
				data.dst_nodeid, data.src_nodeid, data.first_item_index, data.last_item_index );
	}
	WRITE_LOG(LOG_DEBUG, "write_range_request sending complete\n" );
}

/*
 *READ waiting_results x struct sort_result*/
struct sort_result*
read_sort_result( int fdr, int waiting_results ){
	if ( !waiting_results ) return NULL;
	struct sort_result *results = malloc( SRC_NODES_COUNT*sizeof(struct sort_result) );
	for ( int i=0; i < waiting_results; i++ ){
		read( fdr, (char*) &results[i], sizeof(struct sort_result) );
	}
	return results;
}

void
print_request_data_array( struct request_data_t* const range, int len ){
	for ( int j=0; j < len; j++ )
	{
		WRITE_FMT_LOG(LOG_DEBUG, "SEQUENCE N:%d, dst_pid=%d, src_pid=%d, findex %d, lindex %d \n",
				j, range[j].dst_nodeid, range[j].src_nodeid,
				range[j].first_item_index, range[j].last_item_index );
	}
}

