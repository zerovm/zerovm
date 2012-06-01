/*
 * dsort.h
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef DSORT_H_
#define DSORT_H_

#include "sort.h"
#include "distr_common.h"

int
sortresult_comparator( const void *m1, const void *m2 );
int
histogram_srcid_comparator( const void *m1, const void *m2 );

HistogramArrayPtr
alloc_histogram_array_get_len(
		const BigArrayPtr array, int offset, const int array_len, int step, int *histogram_len );

#endif /* DSORT_H_ */
