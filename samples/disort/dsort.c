/*
 * dsort.c
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */



#include "dsort.h"
#include "stdio.h"
#include "distr_common.h"

#include <stdlib.h>
#include <string.h>

int
sortresult_comparator( const void *m1, const void *m2 )
{
	const struct sort_result *t1= (struct sort_result* const)(m1);
	const struct sort_result *t2= (struct sort_result* const)(m2);

	if ( t1->nodeid < t2->nodeid )
		return -1;
	else if ( t1->nodeid > t2->nodeid )
		return 1;
	else return 0;
	return 0;
}

int
histogram_srcid_comparator( const void *m1, const void *m2 ){

	const struct Histogram *t1= (struct Histogram* const)(m1);
	const struct Histogram *t2= (struct Histogram* const)(m2);

	if ( t1->src_nodeid < t2->src_nodeid )
		return -1;
	else if ( t1->src_nodeid > t2->src_nodeid )
		return 1;
	else return 0;
	return 0;
}


HistogramArrayPtr
alloc_histogram_array_get_len(
		const BigArrayPtr array, int offset, const int array_len, int step, int *histogram_len ){
	*histogram_len = array_len/step;
	if ( *histogram_len * step < array_len )
		++*histogram_len;
	int h = 0, i = 0;
	HistogramArrayItem histogram_item;
	memset(&histogram_item, '\0', sizeof(histogram_item));
	HistogramArrayPtr histogram_array = malloc( sizeof(HistogramArrayItem) * *histogram_len );
	for( i=0, h=0; i < array_len && h < *histogram_len; i+=step ){
		histogram_item.item_index = i+offset;
		histogram_item.item = array[i+offset];
		histogram_item.last_item_index = histogram_item.item_index + step -1;
		histogram_array[h++] = histogram_item;
	}
	//add histogram item with histogram_step < step
	if ( histogram_item.item_index < array_len-1 && h < *histogram_len ){
		histogram_item.item_index = offset+ array_len-1;
		histogram_item.item = array[offset+array_len-1];
		histogram_item.last_item_index = histogram_item.item_index + array_len-1-i-1;
		histogram_array[h] = histogram_item;
	}
	return histogram_array;
}


