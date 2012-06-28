/*
 * bitonic_sort.h
 *
 *  Created on: 29.05.2012
 *      Author: yaroslav
 */

#ifndef BITONIC_SORT_H_
#define BITONIC_SORT_H_

/*optimization for CPU cache*/
#define DEFAULT_CHUNK_SIZE		64
/*alloc alignment*/
#define SSE2_ALIGNMENT			16

void bitonic_sort_chunked(float *array, uint32_t items_count, float *extra_buf, uint32_t chunk_size);

#endif /* BITONIC_SORT_H_ */
