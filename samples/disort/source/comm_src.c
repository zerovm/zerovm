/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm.h"
#include "comm_src.h"
#include "dsort.h"
#include "defines.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>


void
init_request_data_array( struct request_data_t *req_data, int len ){
	for ( int j=0; j < len; j++ ){
		req_data[j].src_nodeid = 0;
		req_data[j].dst_nodeid = 0;
		req_data[j].first_item_index = 0;
		req_data[j].last_item_index = 0;
	}
}

/*writing data
 *WRITE 1x uint32_t, crc*/
void
write_crc(int fdw, uint32_t crc ){
	write_channel(fdw, (const char*)&crc, sizeof(crc) );
}

/*writing data
 * 1x struct packet_data_t[EPACKET_HISTOGRAM]
 * array_len x HistogramArrayItem*/
void
write_histogram( int fdw, const struct Histogram *histogram ){
	size_t array_size = sizeof(HistogramArrayItem)*(histogram->array_len);
	struct packet_data_t t;
	t.type = EPACKET_HISTOGRAM;
	t.src_nodeid = histogram->src_nodeid;
	t.size = array_size;
	WRITE_FMT_LOG(LOG_DEBUG, "write histogram: fd=%d, size=%d, type=%d, src_nodeid=%d\n", fdw,
			(int)t.size, t.type, t.src_nodeid);

	/*writing to pipeline whole packet*/
	write_channel(fdw, (const char*)&t, sizeof(t));
	write_channel(fdw, (const char*)histogram->array, array_size);
	WRITE_LOG(LOG_DEBUG, "write histogram OK\n");
}

/*i/o 2 files
 *READ  1x struct request_data_t
 *WRITE 1x char, reply
 *READ  1x char is_complete flag, if 0
 *WRITE 1x int, nodeid
 *READ  1x char, reply
 *WRITE 1x size_t, array_len
 *READ  1x char, reply
 *WRITE array_len x HistogramArrayItem, histograms array
 * @return 1-should receive again, 0-complete request - it should not be listen again*/
int
read_requests_write_detailed_histograms( int fdr, int fdw, int nodeid, const BigArrayPtr source_array, int array_len){
	int is_complete = 0;
	do {
		WRITE_FMT_LOG(LOG_DEBUG, "Reading from file %d detailed histograms request\n", fdr );fflush(0);
		/*receive data needed to create histogram using step=1,
		actually requested histogram should contains array items range*/
		struct request_data_t received_histogram_request;
		read_channel(fdr, (char*) &received_histogram_request, sizeof(struct request_data_t) );
		read_channel(fdr, (char*) &is_complete, sizeof(is_complete) );

		int histogram_len = 0;
		//set to our offset, check it
		int offset = min(received_histogram_request.first_item_index, array_len-1 );
		int requested_length = received_histogram_request.last_item_index - received_histogram_request.first_item_index;
		requested_length = min( requested_length, array_len - offset );

		HistogramArrayPtr histogram = alloc_histogram_array_get_len( source_array, offset, requested_length, 1, &histogram_len );

		size_t sending_array_len = histogram_len;
		/*Response to request, entire reply contains requested detailed histogram*/
		write_channel(fdw, (const char*)&nodeid, sizeof(int) );
		write_channel(fdw, (const char*)&sending_array_len, sizeof(size_t) );
		write_channel(fdw, (const char*)histogram, histogram_len*sizeof(HistogramArrayItem) );
		free( histogram );
		WRITE_FMT_LOG(LOG_DEBUG, "histograms wrote into file %d\n", fdw );
	}while(!is_complete);
	return is_complete;
}



/*READ 1x struct packet_data_t {type EPACKET_SEQUENCE_REQUEST}
 *READ 1x struct request_data_t
 * @param dst_pid destination process should receive ranges*/
int
read_range_request( int fdr, struct request_data_t* sequence ){
	int len = 0;

	WRITE_FMT_LOG(LOG_DEBUG, "reading range  request from %d (EPACKET_SEQUENCE_REQUEST)\n", fdr);
	struct packet_data_t t;
	t.type = EPACKET_UNKNOWN;
	read_channel( fdr, (char*)&t, sizeof(t) );
	WRITE_FMT_LOG(LOG_DEBUG, "readed packet: type=%d, size=%d, src_node=%d\n", t.type, (int)t.size, t.src_nodeid );

	if ( t.type == EPACKET_SEQUENCE_REQUEST )
	{
		struct request_data_t *data = NULL;
		for ( int j=0; j < t.size; j++ ){
			data = &sequence[j];
			read_channel(fdr, (char*)data, sizeof(struct request_data_t));
			WRITE_FMT_LOG(LOG_DEBUG, "recv range request %d %d %d\n", data->src_nodeid, data->first_item_index, data->last_item_index );
		}
	}
	else{
		WRITE_LOG(LOG_ERR,"channel_recv_sequences_request::packet Unknown\n");
		assert(0);
	}
	return len;
}

/*i/o
 *WRITE 1x struct packet_data_t [EPACKET_RANGE]
 *WRITE array_len x BigArrayItem, array*/
void
write_sorted_ranges( int fdw, const struct request_data_t* sequence, const BigArrayPtr src_array ){
	const int array_len = sequence->last_item_index - sequence->first_item_index + 1;
	const BigArrayPtr array = src_array+sequence->first_item_index;
	WRITE_FMT_LOG(LOG_DEBUG, "Sending array_len=%d; min=%u, max=%u\n", array_len, array[0], array[array_len-1]);

	/*write header*/
	struct packet_data_t t;
	t.size = array_len*sizeof(BigArrayItem);
	t.type = EPACKET_RANGE;
	WRITE_FMT_LOG(LOG_DEBUG, "writing to %d: size=%d, type=%d (EPACKET_RANGE)\n", fdw, (int)t.size, t.type );
	WRITE_FMT_LOG(LOG_DEBUG, "sizeof(struct packet_data_t)=%d\n", (int)sizeof(struct packet_data_t));

	write_channel(fdw, (const char*) &t, sizeof(struct packet_data_t));
	write_channel(fdw, (const char*) array, t.size);
	WRITE_LOG(LOG_DEBUG, "Reply from receiver OK;\n");
}



