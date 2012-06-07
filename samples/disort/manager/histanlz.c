/*
 * histanlz.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */


#include "defines.h"
#include "histanlz.h"
#include "comm_man.h"

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


void
init_worker( struct histogram_worker* worker ){
	worker->detailed_histogram.array = NULL;
	worker->helper.begin_histogram_index = 0;
	worker->helper.end_histogram_index = 0;
	worker->helper.begin_detailed_histogram_index = 0;
	worker->helper.end_detailed_histogram_index = 0;
	worker->helper.begin_offset = 0;
	worker->current_histogram_complete = 0;
}


void
check_remove_detailed_histogram( struct histogram_worker* worker ){
	if ( worker->detailed_histogram.array ){
		HistogramArrayItem current_item = worker->detailed_histogram.array[worker->helper.end_detailed_histogram_index];
		int index = 0;
		for ( int i=worker->helper.end_histogram_index; i < worker->histogram.array_len; i++ ){
			if ( worker->histogram.array[i].item_index >= current_item.item_index ){
				index = i;
				break;
			}
		}
		/*if current item of detailed_histogram is last item from detailed_histogram can be synchronized
				e.g. equal to one of big histogram items then synchronize it;*/
		if ( worker->histogram.array[index].item_index == current_item.item_index ){
			worker->helper.begin_offset = worker->detailed_histogram.array[ worker->helper.begin_detailed_histogram_index ].item_index;
			worker->helper.begin_histogram_index = worker->helper.end_histogram_index = index;
			/*detailed histogram currently no needed, discard detailed histogram*/
			free( worker->detailed_histogram.array );
			worker->detailed_histogram.array = NULL;
		}
	}
}


void
init_histogram( struct histogram_worker* worker ){
	worker->helper.begin_offset = 0;
	if ( worker->detailed_histogram.array ){
		worker->helper.begin_histogram_index = -1; //uninitialized can't be used
		worker->helper.begin_detailed_histogram_index = worker->helper.end_detailed_histogram_index;
	}else{
		worker->helper.begin_histogram_index = worker->helper.end_histogram_index;
	}
}

void
set_detailed_histogram( struct histogram_worker* worker, struct Histogram* detailed_histogram  ){
	assert( !worker->detailed_histogram.array ); /*test: should not be previous detailed histograms*/
	if ( detailed_histogram && detailed_histogram->array_len ){
		int big_histogram_first_item_index = worker->histogram.array[worker->helper.end_histogram_index].item_index;
		/*test: 0 item index of 'detailed histogram' should be equal to last item index of histogram*/
		assert( detailed_histogram->array[0].item_index == big_histogram_first_item_index );
		free(worker->detailed_histogram.array);
		worker->detailed_histogram.array = NULL;
		worker->detailed_histogram = *detailed_histogram;
		/*pointing to begin of detailed histogram*/
		worker->helper.begin_detailed_histogram_index = worker->helper.end_detailed_histogram_index=0;
	}
}

void
set_next_histogram( struct histogram_worker* worker ){
	if ( worker->detailed_histogram.array ){
		if ( worker->helper.end_detailed_histogram_index+1 == worker->detailed_histogram.array_len )
			worker->current_histogram_complete = 1; //flag
		if ( ! worker->current_histogram_complete )
			worker->helper.end_detailed_histogram_index++;
	}
	else
		worker->helper.end_histogram_index++;
}


HistogramArrayPtr
value_at_cursor_histogram( struct histogram_worker* worker ){
	struct Histogram *histogram = &worker->histogram;
	int *end_histogram_index = &worker->helper.end_histogram_index;
	if ( worker->detailed_histogram.array ){
		histogram = &worker->detailed_histogram;
		end_histogram_index = &worker->helper.end_detailed_histogram_index;
	}

	if ( !worker->current_histogram_complete && *end_histogram_index < histogram->array_len){
		return &histogram->array[ *end_histogram_index ];
	}
	return 0;
}


int
length_current_histogram( const struct histogram_worker* worker ){
	const struct Histogram* histogram = &worker->histogram;
	const int *end_histogram_index = &worker->helper.end_histogram_index;
	if ( worker->detailed_histogram.array ){
		histogram = &worker->detailed_histogram;
		end_histogram_index = &worker->helper.end_detailed_histogram_index;
	}
	int len= histogram->array[ *end_histogram_index ].last_item_index - histogram->array[ *end_histogram_index ].item_index + 1;
	//WRITE_FMT_LOG(LOG_DEBUG, "item_index=%d, last_item_index=%d",
	//		histogram->array[ *end_histogram_index ].item_index,
	//		histogram->array[ *end_histogram_index ].last_item_index);
	//WRITE_FMT_LOG(LOG_DEBUG, "end_histogram_index=%d, length_current_histogram=%d", *end_histogram_index, len);
	return len;
}

void
get_begin_end_histograms_item_indexes( const struct histogram_worker* worker, int *first_item_index, int *end_item_index ){
	int begin_detail_index = worker->helper.begin_detailed_histogram_index;
	int begin_index = worker->helper.begin_histogram_index;
	if ( worker->detailed_histogram.array )	{
		int min1 = worker->detailed_histogram.array[ begin_detail_index ].item_index;
		if ( worker->helper.begin_offset > 0 ){
			min1 = min( worker->helper.begin_offset, worker->detailed_histogram.array[ begin_detail_index ].item_index );
		}
		if ( begin_index != -1 ){
			min1 = min( min1, worker->histogram.array[ begin_index ].item_index );
		}
		*first_item_index = min1;
		*end_item_index = worker->detailed_histogram.array[worker->helper.end_detailed_histogram_index].item_index;
		if ( worker->current_histogram_complete )
			*end_item_index = *end_item_index+1;
	}
	else
	{
		if ( worker->helper.begin_offset > 0 ){
			*first_item_index = min( worker->helper.begin_offset, worker->histogram.array[ begin_index ].item_index );
		}
		else
			*first_item_index = worker->histogram.array[ begin_index ].item_index;
		*end_item_index = worker->histogram.array[worker->helper.end_histogram_index].item_index;
	}
}

int
size_all_processed_histograms( const struct histogram_worker* array_workers, int array_len ){
	int count = 0;
	int begin_index = 0;
	int end_index = 0;
	for ( int i=0; i < array_len; i++ )	{
		const struct histogram_worker* worker = &array_workers[i];
		begin_index = 0;
		end_index = 0;
		get_begin_end_histograms_item_indexes( worker, &begin_index, &end_index );
		count += end_index - begin_index;
	}
	return count;
}


/*@return two dimensional array, where is result[i][j] an item representing range of data*/
struct request_data_t**
alloc_range_request_analize_histograms( const int single_src_items_count, int nodeid,
		const struct Histogram *histograms_array, size_t len ){
	struct request_data_t **result = NULL;
	struct histogram_worker workers[len];
	for ( int i=0; i < len; i++ ){
		workers[i].histogram = histograms_array[i];
		init_worker(&workers[i]);
	}
	int destination_index = 0;
	int source_index_of_histogram = -1;
	int allow_check_remove_detailed_hitogram = 0;
	do{
		int last_histograms_requested = 0;
		int range_count = 0; /*items count processed for all destinations, max=ARRAY_ITEMS_COUNT*len*/
		if ( destination_index > 0 ){
			for (int j=0; j < len; j++){
				init_histogram( &workers[j] );
			}
		}
		while( range_count < single_src_items_count ){
			source_index_of_histogram = -1;

			/*in case when histogram item of big histogram is equal to item of detailed histogram
			then now switch from detailed to big histogram */
			for (int i=0; i < len; i++){
				/*Last requested detailed histograms should not be deleted to use it's data to completion of Analize*/
				if ( !last_histograms_requested && allow_check_remove_detailed_hitogram )
					check_remove_detailed_histogram( &workers[i] );
			}

			/*search histogram in array of histogram which terms to condition*/
			/*cycle for histograms*/
			HistogramArrayItem min_value; min_value.item = 0;
			HistogramArrayPtr current_value;
			for ( int i=0; i < len; i++ ){
				current_value = value_at_cursor_histogram( &workers[i] );
				if ( !current_value ) continue;
				/*set first item as default minimum value*/
				if ( -1 == source_index_of_histogram ){
					min_value = *current_value;
				}
				//check both current begin & end are in minimum range
				if ( current_value->item <= min_value.item )
				{
					min_value = *current_value;
					source_index_of_histogram = i; /*save source index to use this histogram*/
				}
			}
#ifdef DEBUG
			assert( -1 != source_index_of_histogram );
#endif
			set_next_histogram( &workers[source_index_of_histogram] ); /*move cursor to next histogram*/
			range_count = size_all_processed_histograms( workers, len );

			//if up to end of ARRAY_ITEMS_COUNT range less than len histograms
			//so request histograms with step=1
			int histogram_len = length_current_histogram( &workers[source_index_of_histogram] );
			if ( !workers[source_index_of_histogram].detailed_histogram.array &&
				 range_count + len*histogram_len >= single_src_items_count)
			{
				last_histograms_requested = destination_index+1 >= len;
				WRITE_FMT_LOG(LOG_DEBUG, "#%d Detailed Histograms recv start", destination_index );
				WRITE_FMT_LOG(LOG_DEBUG, "histogram_len=%d, len=%d", (int)histogram_len, (int)len);
				request_assign_detailed_histogram( histogram_len, single_src_items_count, nodeid,
						workers, len, last_histograms_requested );

				WRITE_FMT_LOG(LOG_DEBUG, "#%d Detailed Histograms received", destination_index );
				allow_check_remove_detailed_hitogram = 0;
			}
		} //while
		/*save range data based on histograms*/
		if ( !result )
			result = malloc( sizeof(struct request_data_t*)*len ); /*alloc memory for pointers*/
		result[destination_index] = malloc( sizeof(struct request_data_t)*len ); /*alloc memory for one-dimension array*/
		/*save results*/
		for (int j=0; j < len; j++){
			int first_item_index = 0;
			int last_item_index = 0;
			get_begin_end_histograms_item_indexes( &workers[j], &first_item_index, &last_item_index );
			result[destination_index][j].first_item_index = first_item_index;
			result[destination_index][j].last_item_index = last_item_index-1;
			result[destination_index][j].src_nodeid = workers[j].histogram.src_nodeid;
			for (int k=0; k < SRC_NODES_COUNT; k++)
				if ( histograms_array[j].src_nodeid == (FIRST_SOURCE_NODEID+k)  )
				{
					result[destination_index][j].dst_nodeid = FIRST_DEST_NODEID+k;
				}
		}

		allow_check_remove_detailed_hitogram = 1;
		destination_index++;
	}while( destination_index < len );

	for ( int i=0; i < len; i++ ){
		free(workers[i].detailed_histogram.array);
	}

	return result;
}



void
request_assign_detailed_histogram( int current_histogram_len, int single_src_items_count, int nodeid,
		struct histogram_worker* workers, int array_len, int last_request ){
	struct request_data_t request_detailed_histogram[array_len];
	/*Prepare request_detailed_histogram to do request*/
	for (int i=0; i < array_len; i++){
		WRITE_FMT_LOG(LOG_DEBUG,  "i=%d, workers[i].helper.end_histogram_index=%d",
				i, workers[i].helper.end_histogram_index );

		int start_index = workers[i].histogram.array[workers[i].helper.end_histogram_index].item_index;

		request_detailed_histogram[i].dst_nodeid = workers[i].histogram.src_nodeid;
		request_detailed_histogram[i].src_nodeid = nodeid;
		request_detailed_histogram[i].first_item_index = start_index;
		request_detailed_histogram[i].last_item_index =
				min(start_index + current_histogram_len * array_len, single_src_items_count );

		WRITE_FMT_LOG(LOG_DEBUG,  "Want %d range(%d, %d)",
				request_detailed_histogram[i].dst_nodeid,
				request_detailed_histogram[i].first_item_index,
				request_detailed_histogram[i].last_item_index ); fflush(0);
	}

	/////////////////

	for( int i=0; i < array_len; i++ ){
		int fdr = i+MANAGER_FD_READ_D_HISTOGRAM_REQ_START;
		int fdw = i+MANAGER_FD_WRITE_D_HISTOGRAM_REP_START;
		struct Histogram *detld_histogram = reqrep_detailed_histograms_alloc(
				fdw, fdr, nodeid, &request_detailed_histogram[i], last_request );
		/*save received detailed histograms into workers array, transfers pointer ownership*/
		set_detailed_histogram( &workers[i], detld_histogram );
	}
	/////////////////
}


