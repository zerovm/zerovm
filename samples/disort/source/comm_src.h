/*
 * comm_src.h
 *
 *  Created on: 30.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef COMM_SRC_H_
#define COMM_SRC_H_

#include "distr_common.h"

void
init_request_data_array( struct request_data_t *req_data, int len );

void
write_crc(int fdw, uint32_t crc );

void
write_histogram(int fdw, const struct Histogram *histogram );

int
read_requests_write_detailed_histograms( int fdr, int fdw, int nodeid, const BigArrayPtr source_array, int array_len);

int
read_range_request( int fdr, struct request_data_t* sequence );

void
write_sorted_ranges( int fdw, const struct request_data_t* sequence, const BigArrayPtr src_array);

#endif /* COMM_SRC_H_ */
