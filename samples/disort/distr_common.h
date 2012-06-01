/*
 * distr_sort.h
 *
 *  Created on: 29.04.2012
 *      Author: YaroslavLitvinov
 */

#ifndef DISTR_SORT_H_
#define DISTR_SORT_H_

#include "sort.h" //BigArrayItem
#include <sys/types.h> //pid_t

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

typedef struct histogram_item_t *HistogramArrayPtr;
typedef struct histogram_item_t HistogramArrayItem;

/*Identifiers of packets sending beetwen nodes*/
enum packet_t { EPACKET_UNKNOWN=-1, EPACKET_HISTOGRAM, EPACKET_SEQUENCE_REQUEST,
	EPACKET_RANGE, EPACKET_RANGE_PART };

struct histogram_item_t
{
	int item_index;
	int last_item_index;
	BigArrayItem item;
};


struct Histogram{
	int src_nodeid;
	size_t array_len;
	HistogramArrayPtr array;
};


struct request_data_t{
	int first_item_index;
	int last_item_index;
	int src_nodeid;
	int dst_nodeid;
};


struct node_pid_t{
	pid_t src_nodeid;
	pid_t dst_nodeid;
};

struct sort_result{
	int nodeid;
	BigArrayItem min;
	BigArrayItem max;
	uint32_t crc;
};

/**It used by sorting protocol*/
struct packet_data_t{
	int type; //packet_t enum
	size_t size; //size of next packet
	int src_nodeid;
};

#endif /* DISTR_SORT_H_ */
