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

#define BASE_HISTOGRAM_STEP 1000
#define ARRAY_ITEMS_COUNT 1000000
#define SRC_NODES_COUNT 5

/*Currently up to 50src and 50dst nodes supported,
 * FD values should be unchanged for less or equal nodes count*/
#define FIRST_SOURCE_NODEID (2)
#define LAST_SOURCE_NODEID (FIRST_SOURCE_NODEID+SRC_NODES_COUNT-1)
#define FIRST_DEST_NODEID (52)
#define LAST_DEST_NODEID (FIRST_DEST_NODEID+SRC_NODES_COUNT-1)

#define MANAGER_FD_READ_HISTOGRAM 3
#define MANAGER_FD_WRITE_D_HISTOGRAM_REQ 4
#define MANAGER_FD_READ_D_HISTOGRAM_REQ 54
#define MANAGER_FD_WRITE_RANGE_REQUEST 104
#define MANAGER_FD_READ_SORT_RESULTS 154
#define MANAGER_FD_READ_CRC 155

#define SOURCE_FD_WRITE_HISTOGRAM 3
#define SOURCE_FD_READ_D_HISTOGRAM_REQ 4
#define SOURCE_FD_WRITE_D_HISTOGRAM_REQ 5
#define SOURCE_FD_READ_SEQUENCES_REQ 6
#define SOURCE_FD_WRITE_SORTED_RANGES_REQ 7
#define SOURCE_FD_READ_SORTED_RANGES_REQ 57
#define SOURCE_FD_WRITE_CRC 107

#define DEST_FD_READ_SORTED_RANGES_REP 3
#define DEST_FD_WRITE_SORTED_RANGES_REP 4
#define DEST_FD_WRITE_SORT_RESULT 5




#endif /* DEFINES_H_ */
