/*
 * bitonic_sort.h
 *
 *  Created on: 29.05.2012
 *      Author: yaroslav
 */

#ifndef BITONIC_SORT_H_
#define BITONIC_SORT_H_

#define DEFAULT_CHUNK_SIZE (1u << 18)

void bitonic_sort_chunked(float *dest, uint32_t items_count, float *src, uint32_t chunk_size);

#endif /* BITONIC_SORT_H_ */
