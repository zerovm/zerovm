/*
 * histanlz.h
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef HISTANLZ_H_
#define HISTANLZ_H_

#include "distr_common.h"
#include <sys/types.h>


struct histogram_helper_t{
	int begin_offset;
	int begin_histogram_index; //First histogram in range
	int end_histogram_index; //Last histogram in range
	int begin_detailed_histogram_index; //First histogram in range
	int end_detailed_histogram_index; //Last histogram in range
};


struct histogram_worker{
	struct Histogram histogram;
	struct Histogram detailed_histogram;
	struct histogram_helper_t helper;
	int current_histogram_complete;
};

void
init_worker( struct histogram_worker* worker );

void
check_remove_detailed_histogram( struct histogram_worker* worker );

void
init_histogram( struct histogram_worker* worker );

void
set_detailed_histogram( struct histogram_worker* worker, struct Histogram* detailed_histogram  );

void
set_next_histogram( struct histogram_worker* worker );

HistogramArrayPtr
value_at_cursor_histogram( struct histogram_worker* worker );

int
length_current_histogram( const struct histogram_worker* worker );

void
get_begin_end_histograms_item_indexes( const struct histogram_worker* worker, int *first_item_index, int *end_item_index );

int
size_all_processed_histograms( const struct histogram_worker* array_workers, int array_len );


struct request_data_t**
alloc_range_request_analize_histograms( const int single_src_items_count, pid_t pid,
		const struct Histogram *histograms_array, size_t len );

void
request_assign_detailed_histogram( int current_histogram_len, int single_src_items_count, int node_id,
		struct histogram_worker* workers, int array_len, int last_request );

#endif /* HISTANLZ_H_ */
