/*
 * program will sort given input file and put sorted elements
 * to the specified output file. no checks will perform
 * usage: sorter <input file name> <output file name>
 *
 * update: usage has been changed. all parameters must be passed via manifest
 *
 * note: input must contain (power of 2) 32-bit unsigned integers
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include "api/zvm.h"

#define LOGFIX /* temporary fix until zrt library will be finished */

#ifdef LOGFIX
#define printf(...)\
do {\
  char msg[4096];\
  sprintf(msg, __VA_ARGS__);\
  log_msg(msg);\
} while (0)
#define fprintf(ch, ...) printf(__VA_ARGS__)
#endif

#if 0
#				define DEBUG
#endif

#define DEFAULT_CHUNK_SIZE 18
#define ELEMENT_SIZE sizeof(uint32_t)

#define CLOCK(...) do {  double t = clock(); __VA_ARGS__; printf("test 2 ");\
  printf("\rSorting time = %.3lfsec\n", (clock() - t)/CLOCKS_PER_SEC);} while(0)
  
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
			printf("%u ", ((uint32_t*)a)[i]);\
		printf("\n");\
	} while (0)

/* show place and print "s" elements array "a" */
#define SHOW_RAW_FUNC(a, s)\
	do {\
		int i;\
		printf("%s -- %d: ", __func__, __LINE__);\
		for(i = 0; i < s; ++i)\
			printf("%u ", ((uint32_t*)a)[i]);\
		printf("\n");\
	} while (0)

/* check if "s" elements of array "a" are sorted */
#define CHECK_RAW(a, s)\
	do {\
		int i;\
		for(i = 1; i < s; ++i)\
			if(((uint32_t*)a)[i-1] > ((uint32_t*)a)[i]) {\
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

/* sort 8 elements using bitonal method */
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

	printf("bitonic_sort_kernel4 test1\n");

	/* load 8 elements to sse registers */
	ma = _mm_load_ps(a);
	mb = _mm_load_ps(b);

	printf("bitonic_sort_kernel4 test2\n");

	/* In-Register sort */
	map = _mm_shuffle_ps(ma, mb, _MM_SHUFFLE(2, 0, 2, 0)); /* 0x88: */
	mbp = _mm_shuffle_ps(ma, mb, _MM_SHUFFLE(3, 1, 3, 1)); /* 0xdd: */
	printf("bitonic_sort_kernel4 test3\n");
	_mm_castps_si128(mbp);
	printf("bitonic_sort_kernel4 test3.1\n");
	_mm_castps_si128(map);
	printf("bitonic_sort_kernel4 test3.2\n");
	_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp));
	printf("bitonic_sort_kernel4 test3.3\n");
	printf("bitonic_sort_kernel4 test3\n");

	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test3.10\n");
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test4\n");
	map = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 3, 1)); /* 0x8d: */
	printf("bitonic_sort_kernel4 test5\n");
	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test6\n");
	map = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(hi, hi, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */
	printf("bitonic_sort_kernel4 test7\n");
	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test8\n");
	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(1, 0, 0, 1)); /* 0x41: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 2, 2, 3)); /* 0xeb: */
	printf("bitonic_sort_kernel4 test9\n");
	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test10\n");
	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(3, 2, 1, 0)); /* 0xe4: */
	mbp = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(1, 0, 3, 2)); /* 0x4e: */
	printf("bitonic_sort_kernel4 test11\n");
	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test12\n");
	map = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(3, 1, 2, 0)); /* 0xd8: */
	mbp = _mm_shuffle_ps(lo, hi, _MM_SHUFFLE(2, 0, 3, 1)); /* 0x8d: */
	printf("bitonic_sort_kernel4 test13\n");
	lo = _mm_castsi128_ps (_mm_min_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	hi = _mm_castsi128_ps (_mm_max_epu32(_mm_castps_si128(map), _mm_castps_si128(mbp)));
	printf("bitonic_sort_kernel4 test14\n");
	map = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(2, 0, 2, 0)); /* 0x88: */
	mbp = _mm_shuffle_ps(hi, lo, _MM_SHUFFLE(3, 1, 3, 1)); /* 0xdd: */
	printf("bitonic_sort_kernel4 test15\n");
	map = _mm_shuffle_ps(map, map, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */
	mbp = _mm_shuffle_ps(mbp, mbp, _MM_SHUFFLE(1, 3, 0, 2)); /* 0x72: */
	printf("bitonic_sort_kernel4 test16\n");
	/* unload sorted elements to memory */
	_mm_store_ps(a, map);
	_mm_store_ps(b, mbp);
	printf("bitonic_sort_kernel4 test17\n");
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
inline void bitonic_merge_kernel16n(float *dest, float *a, float *b /* must not be reversed*/, uint32_t s)
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

	float *last_a = a + s;
	float *last_b = b + s;
	float *last_dest = dest + 2*s;

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
			bitonic_merge_kernel16n(destination + step,
				source + step, source + step + step_size, step_size);
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
	printf("bitonic_sort32 %p\n", d);
	/* sort first 16 */
	bitonic_sort_kernel4(d, d + 4);
	printf("bitonic_sort32 test1\n");
	bitonic_sort_kernel4(d + 8, d + 12);
	printf("bitonic_sort32 test2\n");
	bitonic_merge_kernel8(d, d + 8);
	printf("bitonic_sort32 test3\n");

	/* sort second 16 */
	bitonic_sort_kernel4(d + 16, d + 20);
	printf("bitonic_sort32 test4\n");
	bitonic_sort_kernel4(d + 24, d + 28);
	printf("bitonic_sort32 test5\n");
	bitonic_merge_kernel8(d + 16, d + 24);
	printf("bitonic_sort32 test6\n");
	/* merge */
	bitonic_merge_kernel16n(d, d, d + 16, 16);

	printf("bitonic_sort32 CHECK_RAW\n");
	CHECK_RAW(d, 32);
	printf("bitonic_sort32 OK\n");
}

/* sort given chunk (2^chunk_size) of data */
void bitonic_sort(float *d, uint32_t s, float *buf)
{
	uint32_t i;

	printf("bitonic_sort\n");
	/* sort individual 32 strips */
	for (i = 0; i < s ; i += 32){
		printf("i= %u, s=%u\n", i, s);
		bitonic_sort32(d + i);
	}

	/* merge 32-long strips */
	bitonic_merge(d, s, buf, 32);

	CHECK_RAW(d, s);
}

/* main sort routine. chop all data to chunks, sort and merge */
void bitonic_sort_chunked(float *d, uint32_t s, float *buf, uint32_t chunk_size)
{
	uint32_t i;
	printf("bitonic_sort_chunked %u\n", chunk_size);
	for (i = 0; i < s ; i += chunk_size){
		printf("i= %u, buf+i=%p\n", i, buf+i);
		bitonic_sort(d + i, chunk_size, buf + i);
	}
	bitonic_merge(d, s, buf, chunk_size);
}

/* need for SSE aligned memory access */
void *aligned_malloc(size_t size, size_t align)
{
  void *mem = malloc(size + align + sizeof(void*));
  void **ptr = (void**)(((size_t)mem + align + sizeof(void*)) & ~(align - 1));
  ptr[-1] = mem;
  return ptr;
}

/* replaces free() */
void aligned_free(void *ptr)
{
  free(((void**)ptr)[-1]);
}

int main(int argc, char **argv)
{
	uint32_t  chunk_size = DEFAULT_CHUNK_SIZE;
	uint32_t  cnt;
	uint32_t  *d; /* data to sort */
	uint32_t  *buf; /* extra space to sort */
  struct SetupList setup;
  int retcode = ERR_CODE;

  /* setup */
  retcode = zvm_setup(&setup);
  if(retcode) return retcode;

#ifdef LOGFIX
  /* set up the log */
  retcode = log_set(&setup);
  if(retcode) return retcode;
#endif

  /* check if input map size equal to output map size */
  if(setup.channels[InputChannel].fsize != setup.channels[OutputChannel].fsize)
	{
		printf("size of input and output maps are not equal %llu != %llu\n",
		    setup.channels[InputChannel].fsize, setup.channels[OutputChannel].fsize);
		return 5;
	}  

	cnt = setup.channels[InputChannel].fsize / sizeof(*d);
	if(cnt & (cnt - 1))
	{
		printf("\rwrong number of elements in the input file - [%d]\n", cnt);
		return 6;
	}
	printf("number of elements = %d\n", cnt);
	
	/*
	 * about memory allocation, mapping and alignement:
	 * since we got our data to sort through mapped file
	 * we don't have to allocate memory. moreover zerovm
	 * always align allocated files to 64k bound, so we
	 * don't have to worry about alignement
	 * note: for buf we have to allocate aligned memory :(
	 */
	/*
	 * update: we cannot change input data. all data given
	 * by proxy should be untouched. so we copy everything
	 * from input to output
	 */	
	memcpy((void*)setup.channels[OutputChannel].buffer,
	    (void*)setup.channels[InputChannel].buffer, setup.channels[InputChannel].bsize);
	d = (uint32_t*) setup.channels[OutputChannel].buffer;
  buf = (uint32_t*) aligned_malloc(sizeof(uint32_t) * cnt, 16);
	if (buf == NULL) _eoutput("Can't allocate memory\n");

	/* Bitonic sort */
  printf("data sorting.. ");
	CLOCK(bitonic_sort_chunked((float*)d, cnt, (float*)buf, 1u << chunk_size));
  
	return EXIT_SUCCESS;
}
