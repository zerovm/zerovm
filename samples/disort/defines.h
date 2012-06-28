/*
 * defines.h
 *
 *  Created on: 08.05.2012
 *      Author: YaroslavLitvinov
 */

#ifndef DEFINES_H_
#define DEFINES_H_

/*log section start*/
#define LOG_ERR 0
#define LOG_UI 1
#define LOG_DETAILED_UI 2
#define LOG_DEBUG 3
#define LOG_LEVEL LOG_DEBUG

#define WRITE_FMT_LOG(level, fmt, args...) \
		if (level<=LOG_LEVEL ){\
			fprintf(stderr, fmt, args); \
		}
#define WRITE_LOG(level, str) \
		if (level<=LOG_LEVEL ){\
			fprintf(stderr, "%s\n", str); \
		}
/*log section end*/


#define SOURCE_FILE_FMT "data/%dunsorted.data"
#define DEST_FILE_FMT "data/%dsorted.data"

#define SRC_NODES_COUNT 10
//9999872
#define ARRAY_ITEMS_COUNT 5000000
#define CHUNK_COUNT 1000
#define BASE_HISTOGRAM_STEP (ARRAY_ITEMS_COUNT/CHUNK_COUNT)
/*If MERGE_ON_FLY defined then sorted chunks received by destination nodes will only merged when obtained*/
#define MERGE_ON_FLY

/*Currently up to 10src and 10dst nodes supported,
 * FD values should be unchanged for less or equal nodes count*/
#define FIRST_SOURCE_NODEID (2)
#define LAST_SOURCE_NODEID (FIRST_SOURCE_NODEID+SRC_NODES_COUNT-1)
#define FIRST_DEST_NODEID (12)
#define LAST_DEST_NODEID (FIRST_DEST_NODEID+SRC_NODES_COUNT-1)

#define MANAGER_FD_READ_HISTOGRAM_START 3
#define MANAGER_FD_WRITE_D_HISTOGRAM_REP_START 13
#define MANAGER_FD_READ_D_HISTOGRAM_REQ_START 23
#define MANAGER_FD_WRITE_RANGE_REQUEST_START 33
#define MANAGER_FD_READ_SORT_RESULTS_START 43
#define MANAGER_FD_READ_CRC_START 53

#define SOURCE_FD_WRITE_HISTOGRAM 3
#define SOURCE_FD_WRITE_D_HISTOGRAM_REP 4
#define SOURCE_FD_READ_D_HISTOGRAM_REQ 5
#define SOURCE_FD_READ_SEQUENCES_REQ 6
#define SOURCE_FD_WRITE_SORTED_RANGES_START 7
#define SOURCE_FD_WRITE_CRC 17

#define DEST_FD_READ_SORTED_RANGES_START 3
#define DEST_FD_WRITE_SORT_RESULT 13




#endif /* DEFINES_H_ */
