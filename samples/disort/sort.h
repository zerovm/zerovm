/*
 * sort.h
 *
 *  Created on: 17.03.2012
 *      Author: YaroslavLitvinov
 */

#ifndef SORT_H_
#define SORT_H_

#include <stdint.h> //uint32_t
#include <stddef.h> //size_t

#define CRC_ATOM 10000000

typedef uint32_t* BigArrayPtr;
typedef uint32_t  BigArrayItem;

int
quicksort_BigArrayItem_comparator( const void *m1, const void *m2 );

/*alloc new array, sort specified array data in dependency of supported instructions set:
 * for sse41 is used bitonic sort algorithm, for another cases c quick sort is used.
 * return sorted array allocated in heap, caller is responsible to free it after using*/
BigArrayPtr alloc_sort( BigArrayPtr array, int array_len );

BigArrayPtr alloc_merge( BigArrayPtr left_array, int left_array_len,
		BigArrayPtr right_array, int right_array_len );
BigArrayPtr merge( BigArrayPtr dest_array, BigArrayPtr left_array, int left_array_len,
		BigArrayPtr right_array, int right_array_len );

int test_sort_result( BigArrayPtr unsorted, BigArrayPtr sorted, int len );
uint32_t array_crc( BigArrayPtr array, int len );
BigArrayPtr alloc_copy_array( const BigArrayPtr array, int array_len );




#endif /* SORT_H_ */
