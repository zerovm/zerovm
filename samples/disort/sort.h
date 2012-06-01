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


BigArrayPtr alloc_merge_sort( BigArrayPtr array, int array_len );
BigArrayPtr merge( BigArrayPtr left_array, int left_array_len,
		BigArrayPtr right_array, int right_array_len );
int test_sort_result( BigArrayPtr unsorted, BigArrayPtr sorted, int len );
uint32_t array_crc( BigArrayPtr array, int len );



#endif /* SORT_H_ */
