/*
 * comm_src.c
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */


#include "comm_src.h"
#include "dsort.h"
#include "defines.h"

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
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
	write(fdw, &crc, sizeof(crc) );
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
	WRITE_FMT_LOG(LOG_DEBUG, "write histogram: size=%d, type=%d, src_nodeid=%d\n", (int)t.size, t.type, t.src_nodeid);

	int bytes = 0;
	size_t buf_size = sizeof(t)+array_size;
	/*writing to pipeline whole packet*/
	char *buffer = malloc(buf_size);
	memset(buffer, '\0', buf_size );
	memcpy(buffer, &t, sizeof(t));
	bytes = sizeof(t);
	memcpy(buffer+bytes, histogram->array, array_size);
	bytes = write(fdw, buffer, buf_size);
	WRITE_FMT_LOG(LOG_DEBUG, "w array bytes=%d\n", bytes);
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
	char reply;
	do {
		WRITE_FMT_LOG(LOG_DEBUG, "Reading from file %d detailed histograms request\n", fdr );fflush(0);
		/*receive data needed to create histogram using step=1,
		actually requested histogram should contains array items range*/
		struct request_data_t received_histogram_request;
		int bytes;
		bytes = read(fdr, (char*) &received_histogram_request, sizeof(received_histogram_request) );
		assert(bytes>0);
		WRITE_FMT_LOG(LOG_DEBUG, "r request_data_t bytes=%d\n", bytes);
		bytes = write(fdw, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "w reply bytes=%d\n", bytes);
		bytes = read(fdr, (char*) &is_complete, sizeof(is_complete) );
		WRITE_FMT_LOG(LOG_DEBUG, "r is_complete bytes=%d\n", bytes);

		int histogram_len = 0;
		//set to our offset, check it
		int offset = min(received_histogram_request.first_item_index, array_len-1 );
		int requested_length = received_histogram_request.last_item_index - received_histogram_request.first_item_index;
		requested_length = min( requested_length, array_len - offset );

		HistogramArrayPtr histogram = alloc_histogram_array_get_len( source_array, offset, requested_length, 1, &histogram_len );

		size_t sending_array_len = histogram_len;
		/*Response to request, entire reply contains requested detailed histogram*/
		bytes=write(fdw, &nodeid, sizeof(int) );
		WRITE_FMT_LOG(LOG_DEBUG, "w node_id bytes=%d\n", bytes);
		bytes=read(fdr, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d\n", bytes);
		bytes=write(fdw, &sending_array_len, sizeof(size_t) );
		WRITE_FMT_LOG(LOG_DEBUG, "w sending_array_len bytes=%d\n", bytes);
		bytes=read(fdr, &reply, 1);
		WRITE_FMT_LOG(LOG_DEBUG, "r reply bytes=%d", bytes);
		bytes=write(fdw, histogram, histogram_len*sizeof(HistogramArrayItem) );
		WRITE_FMT_LOG(LOG_DEBUG, "r array histogram_len bytes=%d\n", bytes);
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
	int bytes;
	struct packet_data_t t;
	t.type = EPACKET_UNKNOWN;
	bytes=read( fdr, (char*)&t, sizeof(t) );
	WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d\n", bytes);
	WRITE_FMT_LOG(LOG_DEBUG, "readed packet: type=%d, size=%d, src_node=%d\n", t.type, (int)t.size, t.src_nodeid );

	if ( t.type == EPACKET_SEQUENCE_REQUEST )
	{
		struct request_data_t *data = NULL;
		for ( int j=0; j < t.size; j++ ){
			data = &sequence[j];
			bytes=read(fdr, data, sizeof(struct request_data_t));
			WRITE_FMT_LOG(LOG_DEBUG, "r packet_data_t bytes=%d\n", bytes);
			WRITE_FMT_LOG(LOG_DEBUG, "recv range request %d %d %d\n", data->src_nodeid, data->first_item_index, data->last_item_index );
		}
	}
	else{
		WRITE_LOG(LOG_ERR,"channel_recv_sequences_request::packet Unknown\n");
		assert(0);
	}
	return len;
}

/*i/o 2 files
 *WRITE 1x struct packet_data_t [EPACKET_RANGE]
 *READ 1x char, reply
 *WRITE array_len x BigArrayItem, array
 *READ 1x char, reply*/
void
write_sorted_ranges( int fdw, int fdr, const struct request_data_t* sequence, const BigArrayPtr src_array ){

	const int array_len = sequence->last_item_index - sequence->first_item_index + 1;
	const BigArrayPtr array = src_array+sequence->first_item_index;
	WRITE_FMT_LOG(LOG_DEBUG, "Sending array_len=%d; min=%u, max=%u\n", array_len, array[0], array[array_len-1]);
	int bytes;
	char unused_reply;

	/*write header*/
	struct packet_data_t t;
	t.size = array_len*sizeof(BigArrayItem);
	t.type = EPACKET_RANGE;
	WRITE_FMT_LOG(LOG_DEBUG, "writing to %d: size=%d, type=%d (EPACKET_RANGE)\n", fdw, (int)t.size, t.type );
	WRITE_FMT_LOG(LOG_DEBUG, "sizeof(struct packet_data_t)=%d\n", (int)sizeof(struct packet_data_t));

	/*use buffer to send header+data by single package.
	 *note: only for this case, receiver should read it as 2 messages, using 2 read call one by one*/
	size_t buf_size = sizeof(struct packet_data_t) + t.size;
	char *tempbuffer = malloc(buf_size);
	memset(tempbuffer , '\0', buf_size);
	memcpy(tempbuffer, (char*) &t, sizeof(struct packet_data_t));
	memcpy(tempbuffer+sizeof(struct packet_data_t), array, t.size);
	write(fdw, tempbuffer, buf_size);
	bytes=read( fdr, &unused_reply, 1 );
	WRITE_LOG(LOG_DEBUG, "Reply from receiver OK;\n");
}



