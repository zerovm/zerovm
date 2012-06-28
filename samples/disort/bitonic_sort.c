/*
 *	Apache 2.0 License (TODO add the usual blah-blah)
 *
 *	Credits and copyrights:
 *
 *		Camuel Gilyadov - camuel@gmail.com
 *		Dmitri Bortoq - bortoq@gmail.com
 *		Takeshi Yamamura - linguin.m.s@gmail.com
 *		Jatin Chhugani, Akram Baransi et.al. for inspiration
 *
 *
 *	The program has following functionality:
 *
 *		1. Library for sorting 16-byte aligned & padded memory region of 32bit
 *     	floating point numbers using SSE-accelerated bitonic sort. Inspired by
 *     	academic paper written by Intel guys - Jatin Chhugani et. al.
 *
 *     	2. Same as #1 but for unsigned 32bit integers
 *
 *     	3. Same as #1 but for signed 32bit integers (TODO)
 *
 *     	4. Same as #1-#3 but just as merge-sort component (assuming presorting
 *     	of memory regions to be merged) (TODO for #3)
 *
 *     	5. a wrapper implementing C99 qsort interface and accelerating the
 *     	sorting if required SSE functionality is supported by the processor.
 *     	This wrapper must use realloc and padding if needed. (TODO)
 *
 *     	7. A small test program exercising all the above functionality.
 *
 *	Notes:
 *
 *		1. We considered the idea of using realloc and dropping both
 *		requirements: 16 byte alignment and 16 byte padding. But decided that it
 *		will just confuse the user of library and it is better to reject
 *		outright unaligned and unpadded data and clearly indicate to the library
 *		user how it should be used for its best result.
 *
 *		2. However, for proper qsort support we would be forced to use realloc
 *		and padding. (TODO)
 *
 *	Wish list for contributors:
 *
 *		1. TODO items
 *		2. gtest unit tests
 *		3. Support for 64bit and 128bit integers
 *		4. Support for (key,data) pairs. Data is usually fixed size index or
 *		reference. For performance reasons keys and data are stored in same
 *		order but in separate memory regions.
 *
 */


#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include "bitonic_sort.h"
#include "defines.h" //for logging

#define min(X,Y) ((X) < (Y) ? (X) : (Y))
#define max(X,Y) ((X) > (Y) ? (X) : (Y))

//#define DEBUG

#define _eoutput(_str)\
	do {\
		fprintf(stderr, "Error in %s() at %u: %s\n", __func__, __LINE__, (_str));\
		abort();\
	} while (0)

#ifdef DEBUG
#define _doutput(...) fprintf(stderr, __VA_ARGS__)

/* print "s" elements array "a" */
#define SHOW_RAW(a, s)\
	do {\
		int i;\
		for(i = 0; i < s; ++i)\
			fprintf(stderr, "%u(%p,%d) ", ((uint32_t*)a)[i],a,i);\
		fprintf(stderr, "\n");\
	} while (0)

/* show place and print "s" elements array "a" */
#define SHOW_RAW_FUNC(a, s)\
	do {\
		int i;\
		fprintf(stderr, "%s -- %d: ", __func__, __LINE__);\
		for(i = 0; i < s; ++i)\
			fprintf(stderr, "%u ", ((uint32_t*)a)[i]);\
		printf("\n");\
	} while (0)

/* check if "s" elements of array "a" are sorted */
#define CHECK_RAW(a, s)\
	do {\
		int i;\
		for(i = 1; i < s; ++i)\
			if(((uint32_t*)a)[i-1] > ((uint32_t*)a)[i]) {\
				fprintf(stderr, "%u(%p,%d) ", ((uint32_t*)a)[i-1],a,i-1);\
				fprintf(stderr, "%u(%p,%d) ", ((uint32_t*)a)[i],a,i);\
				SHOW_RAW(a, s);\
				_eoutput("not sorted!");}\
	} while(0)

/* check if "s+s" elements of array "a"+"b" are sorted */
#define CHECK_RAWS(a, b, s)\
	do {\
		CHECK_RAW(a, s);\
		CHECK_RAW(b, s);\
		if(((uint32_t*)a)[s-1] > ((uint32_t*)b)[0]) {\
			SHOW_RAW(a, s);\
			SHOW_RAW(b, s);\
			_eoutput("not sorted!");}\
	} while(0)

#else
#define _doutput(...)
#define SHOW_RAW(a, s)
#define CHECK_RAW(a, s)
#define CHECK_RAWS(a, b, s)
#define SHOW_RAW_FUNC(a, s)
#endif /* DEBUG */


int32_t err_check(float *d, uint32_t s) {
	int32_t i;

	for (i = 0; i < (s - 1); i++) {
		if (d[i] > d[i + 1])
			return -1;
	}

	return 0;
}

/* sort 8 elements using bitonic method */
/* elements are given in 2 arrays (4 and 4),
   result will be returned in the same arrays with a straight order */
inline void bitonic_sort_kernel4(float *a, float *b)
{
	__m128	ma;
	__m128	mb;
	__m128	map;
	__m128	mbp;
	__m128	lo;
	__m128	hi;

	/* load 8 elements to sse registers */
	ma = _mm_load_ps(a);
	mb = _mm_load_ps(b);

	/* In-Register sort */
	map = _mm_shuffle_ps(ma, mb, _MM_SHUFFLE(2, 0, 2, 0)); /* 0x88: */
	mbp = _mm_shuffle_ps(ma, mb, _MM_SHUFFLE(3, 1, 3, 1)); /* 0xdd: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 3, 1)); /* 0x8d: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(1, 0, 0, 1)); /* 0x41: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 2, 2, 3)); /* 0xeb: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(3, 2, 1, 0)); /* 0xe4: */
	mbp = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(1, 0, 3, 2)); /* 0x4e: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(2, 0, 3, 1)); /* 0x8d: */

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));

	map = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 2, 0)); /* 0x88: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 3, 1)); /* 0xdd: */

	map = _mm_shuffle_ps(map, map, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */
	mbp = _mm_shuffle_ps(mbp, mbp, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */

	/* unload sorted elements to memory */
	_mm_store_ps(a, map);
	_mm_store_ps(b, mbp);

	CHECK_RAWS(a, b, 4);
}

/* merge 2 sorted arrays (8 elements each) to 1 sorted array
   return result (16 elements) in the same arrays
   TODO(d'b): replace magic numbers with macro */
inline void bitonic_merge_kernel8core(float *a, float *b /* must be reversed*/)
{
	__m128	map[2];
	__m128	mbp[2];
	__m128	lo[2];
	__m128	hi[2];

	map[0] = _mm_load_ps(a);
	mbp[0] = _mm_load_ps(b);

	map[1] = _mm_load_ps(a + 4);
	mbp[1] = _mm_load_ps(b + 4);

	lo[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(mbp[0])));
	hi[0] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(mbp[0])));

	lo[1] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map[1]), _mm_castps_si128(mbp[1])));
	hi[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map[1]), _mm_castps_si128(mbp[1])));

	map[0] = lo[0];
	map[1] = lo[1];
	mbp[0] = hi[0];
	mbp[1] = hi[1];

	/* L1 processing */
	lo[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	lo[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	hi[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));
	hi[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));

	map[0] = _mm_shuffle_ps(lo[0], lo[1], 0xe4);
	map[1] = _mm_shuffle_ps(lo[0], lo[1], 0x4e);
	mbp[0] = _mm_shuffle_ps(hi[0], hi[1], 0xe4);
	mbp[1] = _mm_shuffle_ps(hi[0], hi[1], 0x4e);

	/* L2 processing */
	lo[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	lo[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	hi[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));
	hi[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));

	map[0] = _mm_shuffle_ps(lo[0], lo[1], 0xd8);
	map[1] = _mm_shuffle_ps(lo[0], lo[1], 0x8d);
	mbp[0] = _mm_shuffle_ps(hi[0], hi[1], 0xd8);
	mbp[1] = _mm_shuffle_ps(hi[0], hi[1], 0x8d);

	/* L3 processing */
	lo[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	lo[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map[0]), _mm_castps_si128(map[1])));
	hi[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));
	hi[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(mbp[0]), _mm_castps_si128(mbp[1])));

	map[0] = _mm_shuffle_ps(lo[1], lo[0], 0x88);
	map[1] = _mm_shuffle_ps(lo[1], lo[0], 0xdd);
	mbp[0] = _mm_shuffle_ps(hi[1], hi[0], 0x88);
	mbp[1] = _mm_shuffle_ps(hi[1], hi[0], 0xdd);

	map[0] = _mm_shuffle_ps(map[0], map[0], 0x72);
	map[1] = _mm_shuffle_ps(map[1], map[1], 0x72);
	mbp[0] = _mm_shuffle_ps(mbp[0], mbp[0], 0x72);
	mbp[1] = _mm_shuffle_ps(mbp[1], mbp[1], 0x72);

	_mm_store_ps(&a[0], map[0]);
	_mm_store_ps(&a[4], map[1]);
	_mm_store_ps(&b[0], mbp[0]);
	_mm_store_ps(&b[4], mbp[1]);

	CHECK_RAWS(a, b, 8);
}

/* prepare (reverse) 2nd 8 elements of given 16 and call merge sort
   return result (16 sorted elements) in the same arrays */
inline void bitonic_merge_kernel8(float *a, float *b /* must not be reversed */)
{
	__m128 mb[3];

	/* Reverse *b */
	mb[0] = _mm_load_ps(b);
	mb[1] = _mm_load_ps(b + 4);
	mb[2] = _mm_shuffle_ps(mb[1], mb[1], 0x1b);
	mb[1] = _mm_shuffle_ps(mb[0], mb[0], 0x1b);
	_mm_store_ps(b, mb[2]);
	_mm_store_ps(b + 4, mb[1]);

	bitonic_merge_kernel8core(a, b);

	CHECK_RAWS(a, b, 8);
}

/* merge "s+s" elements and return sorted result in "dest" array
   TODO(d'b): replace magic numbers with macro */
inline void bitonic_merge_kernel16n(float *dest, float *a, uint32_t sa, float *b /* must not be reversed*/, uint32_t sb)
{
	__m128 ma[4];
	__m128 mb[4];
	__m128 lo[4];
	__m128 hi[4];

#define LOAD16(arg) \
	mb[3] = _mm_load_ps(arg); \
	mb[2] = _mm_load_ps(arg + 4); \
	mb[1] = _mm_load_ps(arg + 8); \
	mb[0] = _mm_load_ps(arg + 12); arg+=16

	float *last_a = a + sa;
	float *last_b = b + sb;
	float *last_dest = dest + sa + sb;

	ma[0] = _mm_load_ps(a); a+=4;
	ma[1] = _mm_load_ps(a); a+=4;
	ma[2] = _mm_load_ps(a); a+=4;
	ma[3] = _mm_load_ps(a); a+=4;

	for(; dest < (last_dest - 16); dest += 16)
	{
		/* Load either a or b */
		if(a < last_a)
		{
			if(b < last_b)
			{
				if(*((uint32_t*)a) < *((uint32_t*)b))
				{
					LOAD16(a);
				} else
				{
					LOAD16(b);
				}
			} else
			{
				LOAD16(a);
			}
		} else
		{
			LOAD16(b);
		}

		/* Reverse *b */
		mb[0] = _mm_shuffle_ps(mb[0], mb[0], 0x1b);
		mb[1] = _mm_shuffle_ps(mb[1], mb[1], 0x1b);
		mb[2] = _mm_shuffle_ps(mb[2], mb[2], 0x1b);
		mb[3] = _mm_shuffle_ps(mb[3], mb[3], 0x1b);

		lo[0] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(ma[0]), _mm_castps_si128(mb[0])));
		hi[0] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(ma[0]), _mm_castps_si128(mb[0])));
		lo[1] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(ma[1]), _mm_castps_si128(mb[1])));
		hi[1] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(ma[1]), _mm_castps_si128(mb[1])));
		lo[2] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(ma[2]), _mm_castps_si128(mb[2])));
		hi[2] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(ma[2]), _mm_castps_si128(mb[2])));
		lo[3] = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(ma[3]), _mm_castps_si128(mb[3])));
		hi[3] = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(ma[3]), _mm_castps_si128(mb[3])));

		_mm_store_ps(&dest[0], lo[0]);
		_mm_store_ps(&dest[4], lo[1]);
		_mm_store_ps(&dest[8], lo[2]);
		_mm_store_ps(&dest[12], lo[3]);
		_mm_store_ps(&dest[16], hi[2]);
		_mm_store_ps(&dest[20], hi[3]);
		_mm_store_ps(&dest[24], hi[0]);
		_mm_store_ps(&dest[28], hi[1]);

		bitonic_merge_kernel8core(dest, dest + 8);
		bitonic_merge_kernel8core(dest + 16, dest + 24);

		ma[0] = _mm_load_ps(&dest[16]);
		ma[1] = _mm_load_ps(&dest[20]);
		ma[2] = _mm_load_ps(&dest[24]);
		ma[3] = _mm_load_ps(&dest[28]);
	}
}

/* merge sorted arrays (32 elements each) and return result in "d" array */
void bitonic_merge(float *d, uint32_t s, float *buf, uint32_t chunk_size)
{
	uint32_t	step;
	uint32_t	step_size;
	float 		*source = d;
	float			*destination = buf;
	float			*temp;

	for (step_size = chunk_size; step_size < s; step_size *= 2)
	{
		for (step = 0; step < s; step += step_size * 2)
		{
			bitonic_merge_kernel16n(
				destination + min(step, s),
				source + min(step, s),
				min(step_size, s - step),
				source + min(step + step_size, s),
				min(step_size,s - min(step + step_size, s)));
		}

		temp = source;
		source = destination;
		destination = temp;
	}

	if(source != d)
		memcpy(d, source, s * sizeof(float));
}

/* sort given 32 elemens */
void inline bitonic_sort32(float *d)
{
	/* sort first 16 */
	bitonic_sort_kernel4(d, d + 4);
	bitonic_sort_kernel4(d + 8, d + 12);
	bitonic_merge_kernel8(d, d + 8);

	/* sort second 16 */
	bitonic_sort_kernel4(d + 16, d + 20);
	bitonic_sort_kernel4(d + 24, d + 28);
	bitonic_merge_kernel8(d + 16, d + 24);

	/* merge */
	bitonic_merge_kernel16n(d, d, 16, d + 16, 16);

	CHECK_RAW(d, 32);
}

/* sort given chunk (2^chunk_size) of data */
void bitonic_sort(float *d, uint32_t s, float *buf)
{
	uint32_t i,j, k;

	/* sort individual 32 strips */
	for (i = 0; (i+32) <= s ; i += 32)
		bitonic_sort32(d + i);

	for (j = i; j < s ; j ++)
		for (k = i; k < s - 1 ; k ++)
			if(*((uint32_t*)(d+k)) > *((uint32_t*)(d+k+1))){
				uint32_t temp = *((uint32_t*)(d+k+1));
				*((uint32_t*)(d+k+1)) = *((uint32_t*)(d+k));
				*((uint32_t*)(d+k)) = temp;
		}

	/* merge 32-long strips */
	bitonic_merge(d, s, buf, 32);

	CHECK_RAW(d, s);
}

/* main sort routine. chop all data to chunks, sort and merge */
void bitonic_sort_chunked(float *d, uint32_t s, float *extra_buf, uint32_t chunk_size)
{
	uint32_t i;
	for (i = 0; i < s ; i += chunk_size)
		bitonic_sort(
				d + i,
				min(chunk_size, s - i),
				extra_buf + i);
	bitonic_merge(d, s, extra_buf, chunk_size);
}
